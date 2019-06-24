//
// Created by Brett Chaldecott on 2019-05-09.
//


#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/block/BlockChainCallbackImpl.hpp"
#include "keto/block/Constants.hpp"
#include "keto/block/BlockSyncManager.hpp"
#include "keto/block_db/BlockChainStore.hpp"
#include "keto/block_db/SignedBlockBatchRequestProtoHelper.hpp"

namespace keto {
namespace block {

    static BlockSyncManagerPtr singleton;

BlockSyncManager::BlockSyncManager() : status(INIT), startTime(0) {

}

BlockSyncManager::~BlockSyncManager() {

}

BlockSyncManagerPtr BlockSyncManager::createInstance() {
    return singleton = BlockSyncManagerPtr(new BlockSyncManager());
}

BlockSyncManagerPtr BlockSyncManager::getInstance() {
    return singleton;
}

void BlockSyncManager::finInstance() {
    singleton.reset();
}

BlockSyncManager::Status BlockSyncManager::getStatus() {
    return this->status;
}


std::time_t BlockSyncManager::getStartTime() {
    return this->startTime;
}

void BlockSyncManager::sync() {
    KETO_LOG_INFO << "[BlockSyncManager::sync] start of the sync";
    if (this->status == WAIT && !isExpired()) {
        return;
    }
    if (!this->tangleHashes.size()) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] attempt to get the last block hash";
        this->tangleHashes = keto::block_db::BlockChainStore::getInstance()->getLastBlockHashs();
    }
    KETO_LOG_INFO << "[BlockSyncManager::sync] loop through the signed blocks";
    keto::block_db::SignedBlockBatchRequestProtoHelper signedBlockBatchRequestProtoHelper;
    for (keto::asn1::HashHelper hash: this->tangleHashes) {
        signedBlockBatchRequestProtoHelper.addHash(hash);
    }

    KETO_LOG_INFO << "[BlockSyncManager::sync] make the block sync request";
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
            keto::server_common::Events::RPC_CLIENT_REQUEST_BLOCK_SYNC,signedBlockBatchRequestProtoHelper));

    KETO_LOG_INFO << "[BlockSyncManager::sync] reset the start time for the sync";
    this->startTime = time(0);
}

keto::proto::SignedBlockBatchMessage  BlockSyncManager::requestBlocks(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    keto::block_db::SignedBlockBatchRequestProtoHelper signedBlockBatchRequestProtoHelper(signedBlockBatchRequest);
    std::vector<keto::asn1::HashHelper> tangledHashes;
    for (int index = 0; index < signedBlockBatchRequestProtoHelper.hashCount(); index++) {
        tangledHashes.push_back(signedBlockBatchRequestProtoHelper.getHash(index));
    }
    return keto::block_db::BlockChainStore::getInstance()->requestBlocks(tangledHashes);
}

keto::proto::MessageWrapperResponse  BlockSyncManager::processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage) {

    keto::proto::MessageWrapperResponse response;
    if (keto::block_db::BlockChainStore::getInstance()->processBlockSyncResponse(signedBlockBatchMessage,BlockChainCallbackImpl())) {
        response.set_result("complete");
        this->status = COMPLETE;
    } else {
        response.set_result("applied");
    }
    response.set_success(true);

    return response;

}


bool BlockSyncManager::isExpired() {
    std::time_t now = time(0);
    return now > (this->startTime + Constants::SYNC_EXPIRY_TIME);
}



}
}