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
#include "keto/router_utils/RpcPeerHelper.hpp"

namespace keto {
namespace block {

    static BlockSyncManagerPtr singleton;

BlockSyncManager::BlockSyncManager(bool enabled) : enabled(enabled), status(INIT), startTime(0) {

}

BlockSyncManager::~BlockSyncManager() {

}

BlockSyncManagerPtr BlockSyncManager::createInstance(bool enabled) {
    return singleton = BlockSyncManagerPtr(new BlockSyncManager(enabled));
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
    if (!isExpired()) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] the timeout has not been reached yet [" << this->startTime << "][" <<
            time(0) << "]";
        return;
    } else if (getStatus() == COMPLETE) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] the processing is now complete ignore further sync requests";
        return;
    }
    //if (!this->tangleHashes.size()) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] attempt to get the last block hash";
        this->tangleHashes = keto::block_db::BlockChainStore::getInstance()->getLastBlockHashs();
    //}
    KETO_LOG_DEBUG << "[BlockSyncManager::sync] loop through the signed blocks : " << this->tangleHashes.size();
    keto::block_db::SignedBlockBatchRequestProtoHelper signedBlockBatchRequestProtoHelper;
    for (keto::asn1::HashHelper hash: this->tangleHashes) {
        KETO_LOG_DEBUG << "[BlockSyncManager::sync] add the block hash to the list : " << hash.getHash(keto::common::HEX);
        signedBlockBatchRequestProtoHelper.addHash(hash);
    }

    KETO_LOG_DEBUG << "[BlockSyncManager::sync] make the block sync request";
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
            keto::server_common::Events::RPC_CLIENT_REQUEST_BLOCK_SYNC,signedBlockBatchRequestProtoHelper));

    KETO_LOG_DEBUG << "[BlockSyncManager::sync] reset the start time for the sync";
    this->startTime = time(0);
}

keto::proto::SignedBlockBatchMessage  BlockSyncManager::requestBlocks(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    keto::block_db::SignedBlockBatchRequestProtoHelper signedBlockBatchRequestProtoHelper(signedBlockBatchRequest);
    std::vector<keto::asn1::HashHelper> tangledHashes;
    KETO_LOG_DEBUG<< "[BlockSyncManager::requestBlocks]" << " Request blocks : " << signedBlockBatchRequestProtoHelper.hashCount();
    for (int index = 0; index < signedBlockBatchRequestProtoHelper.hashCount(); index++) {
        KETO_LOG_DEBUG<< "[BlockSyncManager::requestBlocks]" << " Request block sync for the following block  : " <<
            signedBlockBatchRequestProtoHelper.getHash(index).getHash(keto::common::HEX);
        tangledHashes.push_back(signedBlockBatchRequestProtoHelper.getHash(index));
    }


    return keto::block_db::BlockChainStore::getInstance()->requestBlocks(tangledHashes);
}

keto::proto::MessageWrapperResponse  BlockSyncManager::processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage) {

    keto::proto::MessageWrapperResponse response;
    if (keto::block_db::BlockChainStore::getInstance()->processBlockSyncResponse(signedBlockBatchMessage,BlockChainCallbackImpl())) {
        response.set_result("complete");
        this->status = COMPLETE;
        KETO_LOG_INFO << "[BlockProducer::processBlockSyncResponse]" << " ########################################################";
        KETO_LOG_INFO << "[BlockProducer::processBlockSyncResponse]" << " ######## Synchronization has now been completed ########";
        KETO_LOG_INFO << "[BlockProducer::processBlockSyncResponse]" << " ########################################################";
        if (this->isEnabled()) {
            notifyPeers();
        }

    } else {
        KETO_LOG_DEBUG << "[BlockSyncManager::processBlockSyncResponse] finished process need to trigger the next request.";
        response.set_result("applied");
        this->startTime = 0;
    }
    response.set_success(true);

    return response;

}

void
BlockSyncManager::processRequestBlockSyncRetry() {
    KETO_LOG_DEBUG << "[BlockSyncManager::processRequestBlockSyncRetry] trigger the retry by resetting the start time";
    this->startTime =0;
}

void
BlockSyncManager::notifyPeers() {
    keto::router_utils::RpcPeerHelper rpcPeerHelper;
    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    rpcPeerHelper.setActive(true);

    rpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RpcPeer>(
            keto::server_common::Events::RPC_CLIENT_ACTIVATE_RPC_PEER,rpcPeerHelper));
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RpcPeer>(
            keto::server_common::Events::RPC_SERVER_ACTIVATE_RPC_PEER,rpcPeerHelper));
    KETO_LOG_INFO << "[BlockProducer::processBlockSyncResponse]" << " ########################################################";
    KETO_LOG_INFO << "[BlockProducer::processBlockSyncResponse]" << " ######## Notify peers of status                 ########";
    KETO_LOG_INFO << "[BlockProducer::processBlockSyncResponse]" << " ########################################################";
}

bool
BlockSyncManager::isEnabled() {
    return this->enabled;
}

bool BlockSyncManager::isExpired() {
    std::time_t now = time(0);
    return now > (this->startTime + Constants::SYNC_EXPIRY_TIME);
}



}
}