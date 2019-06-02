/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerResourceManager.cpp
 * Author: ubuntu
 * 
 * Created on February 28, 2018, 11:01 AM
 */

#include "keto/server_common/TransactionHelper.hpp"
#include "keto/rpc_client/PeerResourceManager.hpp"

#include <iostream>

namespace keto {
namespace rpc_client {

thread_local PeerResourcePtr PeerResourceManager::peerResourcePtr;

std::string PeerResourceManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

PeerResourceManager::PeerResourceManager(
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr) :
    dbManagerPtr(dbManagerPtr) {
}

PeerResourceManager::~PeerResourceManager() {
}

void PeerResourceManager::commit() {
    if (peerResourcePtr) {
        peerResourcePtr->commit();
        peerResourcePtr.reset();
    }
}

void PeerResourceManager::rollback() {
    if (peerResourcePtr) {
        peerResourcePtr->rollback();
        peerResourcePtr.reset();
    }
}

PeerResourcePtr PeerResourceManager::getResource() {
    if (!peerResourcePtr) {
        peerResourcePtr = PeerResourcePtr(new PeerResource(dbManagerPtr));
        keto::server_common::enlistResource(*this);
    }
    return peerResourcePtr;
}


}
}
