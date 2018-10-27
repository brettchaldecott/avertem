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
#include "include/keto/router/PeerCache.hpp"

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
    this->entries[rpcPeerHelper.getAccountHashBytes()] = rpcPeerHelper;
}

keto::router_utils::RpcPeerHelper& PeerCache::getPeer(
        const std::vector<uint8_t>& accountHash) {
    return this->entries[accountHash];
}

bool PeerCache::contains(const std::vector<uint8_t>& accountHash) {
    if (this->entries.count(accountHash)) {
        return true;
    }
    return false;
}


}
}