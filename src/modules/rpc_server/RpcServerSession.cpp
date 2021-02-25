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
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/LogManager.hpp"

#include "keto/rpc_server/Constants.hpp"
#include "keto/server_common/StringUtils.hpp"

namespace keto {
namespace rpc_server {

static RpcServerSessionPtr singleton;

namespace ketoEnv = keto::environment;

RpcServerSession::RpcServerSession() {
    // retrieve the configuration
    std::shared_ptr<ketoEnv::Config> config = ketoEnv::EnvironmentManager::getInstance()->getConfig();

    if (config->getVariablesMap().count(Constants::CONFIGURED_PEER_LIST)) {
        this->configuredPeerList = keto::server_common::StringUtils(
                config->getVariablesMap()[Constants::CONFIGURED_PEER_LIST].as<std::string>()).tokenize(",");
    }

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
    std::vector<std::string> peers;
    if (this->configuredPeerList.size()) {
        peers = this->configuredPeerList;
    } else {
        peers = getPeers(account);
    }
    addPeer(account, host);
    return peers;
}

void RpcServerSession::addPeer(const std::vector<uint8_t>& account,
        const std::string& host) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    KETO_LOG_INFO << "[RpcServerSession::addPeer] add the host [" << host << "]";
    if (!this->accountPeerCache.count(account)) {
        if (this->accountPeerList.size() >= Constants::MAX_PEERS) {
            this->accountPeerList.erase(this->accountPeerList.begin());
        }
        this->accountPeerList.push_back(host);
    } else {
        // update the current host
        std::string currentHost = this->accountPeerCache[account];
        for (int index = 0; index < this->accountPeerList.size(); index++) {
            std::string listHost = this->accountPeerList[index];
            if (listHost == currentHost) {
                this->accountPeerList[index] = host;
                break;
            }
        }
    }
    // set the account peer cache
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
