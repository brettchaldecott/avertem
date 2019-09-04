/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerCache.cpp
 * Author: ubuntu
 * 
 * Created on August 25, 2018, 3:14 PM
 */

#include <condition_variable>

#include "keto/router/PeerCache.hpp"
#include "keto/router/Exception.hpp"

namespace keto {
namespace router {

static PeerCachePtr singleton;
    
std::string PeerCache::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

PeerCache::PeerCache() {
}

PeerCache::~PeerCache() {
}

PeerCachePtr PeerCache::init() {
    return singleton = std::shared_ptr<PeerCache>(new PeerCache());
}

void PeerCache::fin() {
    singleton.reset();
}

PeerCachePtr PeerCache::getInstance() {
    return singleton;
}

void PeerCache::addPeer(keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    this->entries[rpcPeerHelper.getAccountHashBytes()] = rpcPeerHelper;
}

void PeerCache::removePeer(keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    this->entries.erase(rpcPeerHelper.getAccountHashBytes());
}

void PeerCache::activateRpcPeer(keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    this->entries[rpcPeerHelper.getAccountHashBytes()].setActive(rpcPeerHelper.isActive());
}


keto::router_utils::RpcPeerHelper& PeerCache::getPeer(
        const std::vector<uint8_t>& accountHash) {
    std::lock_guard<std::mutex> guard(classMutex);
    return this->entries[accountHash];
}

bool PeerCache::contains(const std::vector<uint8_t>& accountHash) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->entries.count(accountHash)) {
        return true;
    }
    return false;
}


std::vector<uint8_t> PeerCache::electPeer(const std::vector<uint8_t>& accountHash) {
    std::vector<std::vector<uint8_t>> peers = getAccounts();
    if (!peers.size()) {
        BOOST_THROW_EXCEPTION(NoPeersRegistered("There are no peers in the cache"));
    } else if (peers.size() == 1 && peers[0] == accountHash) {
        BOOST_THROW_EXCEPTION(NoPeersRegistered("The only peer available is the current master."));
    }

    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,peers.size());
    distribution(stdGenerator);


    std::vector<uint8_t> result;
    do {
        result = peers[distribution(stdGenerator)];
    } while(accountHash == result);
    return result;
}


std::vector<std::vector<uint8_t>> PeerCache::getAccounts() {
    std::lock_guard<std::mutex> guard(classMutex);
    std::vector<std::vector<uint8_t>> result;
    for (std::pair<std::vector<uint8_t>,keto::router_utils::RpcPeerHelper> pair : this->entries) {
        if (pair.second.isActive()) {
            result.push_back(pair.first);
        }
    }
    return result;
}


}
}