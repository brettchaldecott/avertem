/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcServerSession.cpp
 * Author: ubuntu
 * 
 * Created on August 18, 2018, 1:05 PM
 */

#include <iostream>
#include <condition_variable>

#include "keto/rpc_server/RpcServerSession.hpp"

#include "keto/environment/LogManager.hpp"

#include "keto/rpc_server/Constants.hpp"

namespace keto {
namespace rpc_server {

static RpcServerSessionPtr singleton;

RpcServerSession::RpcServerSession() {
}

RpcServerSession::~RpcServerSession() {
}


RpcServerSessionPtr RpcServerSession::init() {
    return singleton = std::make_shared<RpcServerSession>();
}

RpcServerSessionPtr RpcServerSession::getInstance() {
    return singleton;
}

void RpcServerSession::fin() {
    singleton.reset();
}

std::vector<std::string> RpcServerSession::handlePeers(const std::vector<uint8_t>& account,
        const std::string& host) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    std::vector<std::string> peers = getPeers(account);
    addPeer(account,host);
    return peers;
}

void RpcServerSession::addPeer(const std::vector<uint8_t>& account,
        const std::string& host) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    if (!this->accountPeerCache.count(account)) {
        if (this->accountPeerList.size() >= Constants::MAX_PEERS) {
            this->accountPeerList.erase(this->accountPeerList.begin());
        }
        this->accountPeerList.push_back(host);
    }
    this->accountPeerCache[account] = host;
}

std::vector<std::string> RpcServerSession::getPeers(
        const std::vector<uint8_t> account) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    std::set<std::string> result(this->accountPeerList.begin(),this->accountPeerList.end());
    if (this->accountPeerCache.count(account)) {
        if (result.count(this->accountPeerCache[account])) {
            result.erase(this->accountPeerCache[account]);
        }
    }
    return std::vector<std::string>(result.begin(),result.end());
}

std::vector<std::string> RpcServerSession::getPeers(
        const std::vector<std::vector<uint8_t>>& accounts) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    std::vector<std::string> result;
    for (auto const &entry : accounts) {
        if (this->accountPeerCache.count(entry)) {
            result.push_back(this->accountPeerCache[entry]);
        }
    }
    return result;
}


}
}
