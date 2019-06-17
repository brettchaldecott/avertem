/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerStore.cpp
 * Author: ubuntu
 * 
 * Created on March 6, 2018, 3:20 AM
 */

#include <rocksdb/utilities/transaction.h>
#include <google/protobuf/message_lite.h>

#include "keto/rpc_client/PeerStore.hpp"
#include "keto/rpc_client/Constants.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/router_utils/AccountRoutingStoreHelper.hpp"

namespace keto {
namespace rpc_client {

static PeerStorePtr singleton;

std::string PeerStore::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

PeerStore::PeerStore() {
    // re-setup the db routing information and purge the data that is in place.
    dbManagerPtr = std::shared_ptr<keto::rocks_db::DBManager>(
            new keto::rocks_db::DBManager(Constants::DB_LIST));
    peerResourceManagerPtr  =  PeerResourceManagerPtr(
            new PeerResourceManager(dbManagerPtr));
}

PeerStore::~PeerStore() {
    peerResourceManagerPtr.reset();
    dbManagerPtr.reset();
}

PeerStorePtr PeerStore::init() {
    return singleton = PeerStorePtr(new PeerStore());
}

void PeerStore::fin() {
    singleton.reset();
}

PeerStorePtr PeerStore::getInstance() {
    return singleton;
}

// get the peers
void PeerStore::setPeers(const std::vector<std::string>& peers) {
    PeerResourcePtr resource = peerResourceManagerPtr->getResource();
    rocksdb::Transaction* peerTransaction = resource->getTransaction(Constants::PEER_INDEX);
    for (std::string peer : peers) {
        keto::rocks_db::SliceHelper peerHelper(peer);
        peerTransaction->Put(peerHelper,peerHelper);
    }
}

std::vector<std::string> PeerStore::getPeers() {
    PeerResourcePtr resource = peerResourceManagerPtr->getResource();
    rocksdb::Transaction* peerTransaction = resource->getTransaction(Constants::PEER_INDEX);

    rocksdb::ReadOptions readOptions;
    rocksdb::Iterator* iterator = peerTransaction->GetIterator(readOptions);
    iterator->SeekToFirst();
    std::vector<std::string> values;
    while(iterator->Valid()) {
        keto::rocks_db::SliceHelper valueHelper(iterator->key());
        values.push_back((std::string)valueHelper);
        iterator->Next();
    }
    return values;
}

}
}
