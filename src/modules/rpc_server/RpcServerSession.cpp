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

#include <condition_variable>

#include "keto/rpc_server/RpcServerSession.hpp"
#include "include/keto/rpc_server/RpcServerSession.hpp"

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

void RpcServerSession::addPeer(const std::vector<uint8_t>& account,
        const std::string& host) {
    this->accountPeerCache[account] = host;
}

std::vector<std::string> RpcServerSession::getPeers(
        const std::vector<uint8_t> account) {
    std::vector<std::string> result;
    for (auto const &entry : this->accountPeerCache) {
        if (entry.first != account) {
            result.push_back(entry.second);
        }
    }
    return result;
}

std::vector<std::string> RpcServerSession::getPeers(
        const std::vector<std::vector<uint8_t>>& accounts) {
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