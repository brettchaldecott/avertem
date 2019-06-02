/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RouterStore.cpp
 * Author: ubuntu
 * 
 * Created on March 6, 2018, 3:20 AM
 */

#include <rocksdb/utilities/transaction.h>
#include <google/protobuf/message_lite.h>

#include "keto/router_db/RouterStore.hpp"
#include "keto/router_db/Constants.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/router_utils/AccountRoutingStoreHelper.hpp"
#include "keto/router_db/Exception.hpp"
#include "keto/router_db/RouterStore.hpp"

namespace keto {
namespace router_db {

static std::shared_ptr<RouterStore> singleton;

std::string RouterStore::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RouterStore::RouterStore() {
    // re-setup the db routing information and purge the data that is in place.
    dbManagerPtr = std::shared_ptr<keto::rocks_db::DBManager>(
            new keto::rocks_db::DBManager(Constants::DB_LIST,true));
    routerResourceManagerPtr  =  RouterResourceManagerPtr(
            new RouterResourceManager(dbManagerPtr));
}

RouterStore::~RouterStore() {
    routerResourceManagerPtr.reset();
    dbManagerPtr.reset();
}

std::shared_ptr<RouterStore> RouterStore::init() {
    if (singleton) {
        return singleton;
    }
    return singleton = std::shared_ptr<RouterStore>(new RouterStore());
}

void RouterStore::fin() {
    singleton.reset();
}

std::shared_ptr<RouterStore> RouterStore::getInstance() {
    return singleton;
}

bool RouterStore::getAccountRouting(
            const keto::asn1::HashHelper& accountHash,
            keto::proto::RpcPeer& result) {
    RouterResourcePtr resource = routerResourceManagerPtr->getResource();
    rocksdb::Transaction* routerTransaction = resource->getTransaction(Constants::ROUTER_INDEX);
    keto::rocks_db::SliceHelper accountHashHelper(keto::crypto::SecureVectorUtils().copyFromSecure(
        accountHash));
    rocksdb::ReadOptions readOptions;
    std::string value;
    if (rocksdb::Status::OK() != routerTransaction->Get(readOptions,accountHashHelper,&value)) {
        return false;
    }
    result.ParseFromString(value);
    return true;
}


void RouterStore::pushPeerRouting(
        const keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    keto::router_utils::RpcPeerHelper newRpcPeerHelper(rpcPeerHelper);
    RouterResourcePtr resource = routerResourceManagerPtr->getResource();
    rocksdb::Transaction* routerTransaction = resource->getTransaction(Constants::ROUTER_INDEX);
    keto::rocks_db::SliceHelper accountSliceHelper(rpcPeerHelper.getAccountHashBytes());
    std::string value;
    rocksdb::ReadOptions readOptions;
    auto status = routerTransaction->Get(readOptions,accountSliceHelper,&value);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() != status) {
        keto::router_utils::RpcPeerHelper existingEntryRpcPeerHelper(value);
        for (int index = 0; index < existingEntryRpcPeerHelper.numberOfChildren(); index++) {
            newRpcPeerHelper.addChild(*existingEntryRpcPeerHelper.getChild(index));
        }
    }
    keto::rocks_db::SliceHelper rpcPeerSliceHelper(newRpcPeerHelper.toString());
    routerTransaction->Put(accountSliceHelper,rpcPeerSliceHelper);
    for (int index = 0; index < rpcPeerHelper.numberOfChildren(); index++) {
        pushPeerRouting(*rpcPeerHelper.getChild(index));
    }
}


}
}
