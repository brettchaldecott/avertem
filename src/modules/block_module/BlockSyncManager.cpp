//
// Created by Brett Chaldecott on 2019-05-09.
//


#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/block/BlockChainCallbackImpl.hpp"
#include "keto/block/Constants.hpp"
#include "keto/block/BlockSyncManager.hpp"
#include "keto/block/Exception.hpp"
#include "keto/block/BlockProducer.hpp"

#include "keto/block_db/BlockChainStore.hpp"
#include "keto/block_db/SignedBlockBatchRequestProtoHelper.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

namespace keto {
namespace block {

    static BlockSyncManagerPtr singleton;

BlockSyncManager::BlockSyncManager(bool enabled) : enabled(enabled), status(INIT), startTime(0),
    lastClientBlockTimestamp(0), lastServerBlockTimestamp(0) {

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
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->status;
}


std::time_t BlockSyncManager::getStartTime() {
    return this->startTime;
}

void BlockSyncManager::sync() {
    KETO_LOG_INFO << "[BlockSyncManager::sync] start of the sync : " << getStatus();
    waitForExpired();
    KETO_LOG_INFO << "[BlockSyncManager::sync] After expired : " << getStatus();
    if (getStatus() == COMPLETE) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] the processing is now complete ignore further sync requests";
        return;
    }

    KETO_LOG_INFO << "[BlockSyncManager::sync] attempt to get the last block hash";
    this->tangleHashes = keto::block_db::BlockChainStore::getInstance()->getLastBlockHashs();

    //KETO_LOG_DEBUG << "[BlockSyncManager::sync] loop through the signed blocks : " << this->tangleHashes.size();
    keto::block_db::SignedBlockBatchRequestProtoHelper signedBlockBatchRequestProtoHelper;
    for (keto::asn1::HashHelper hash: this->tangleHashes) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] add the block hash to the list : " << hash.getHash(keto::common::HEX);
        signedBlockBatchRequestProtoHelper.addHash(hash);
    }

    try {
        if (requestFromServer()) {
            KETO_LOG_INFO << "[BlockSyncManager::sync] make the block sync request to the server";
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                    keto::server_common::Events::RPC_SERVER_REQUEST_BLOCK_SYNC, signedBlockBatchRequestProtoHelper));
        } else {
            KETO_LOG_INFO << "[BlockSyncManager::sync] make the block sync request to the client first";
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                    keto::server_common::Events::RPC_CLIENT_REQUEST_BLOCK_SYNC, signedBlockBatchRequestProtoHelper));
        }

        //KETO_LOG_DEBUG << "[BlockSyncManager::sync] reset the start time for the sync";
        {
            std::unique_lock<std::mutex> uniqueLock(this->classMutex);
            this->startTime = time(0);
        }
        return;
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[BlockSyncManager::sync]: Failed to request the block sync : " << boost::diagnostic_information(ex,true);
        KETO_LOG_ERROR << "[BlockSyncManager::sync]: cause : " << ex.what();
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[BlockSyncManager::sync]: Failed to request the block sync : " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[BlockSyncManager::sync]: Failed to request the block sync : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[BlockSyncManager::sync]: Failed to request the block sync : " << std::endl;
    }
    scheduledDelayRetry();
}

keto::proto::SignedBlockBatchMessage  BlockSyncManager::requestBlocks(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    //std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (this->getStatus() == INIT) {
        KETO_LOG_DEBUG << "[BlockSyncManager::requestBlocks] This node is currently not unsyncronized and cannot provide data";
        BOOST_THROW_EXCEPTION(keto::block::UnsyncedStateCannotProvideDate());
    }

    // pull the block sync information
    keto::block_db::SignedBlockBatchRequestProtoHelper signedBlockBatchRequestProtoHelper(signedBlockBatchRequest);
    std::vector<keto::asn1::HashHelper> tangledHashes;
    //KETO_LOG_DEBUG<< "[BlockSyncManager::requestBlocks]" << " Request blocks : " << signedBlockBatchRequestProtoHelper.hashCount();
    for (int index = 0; index < signedBlockBatchRequestProtoHelper.hashCount(); index++) {
        KETO_LOG_INFO << "[BlockSyncManager::requestBlocks]" << " Request block sync for the following block  : " <<
            signedBlockBatchRequestProtoHelper.getHash(index).getHash(keto::common::HEX);
        tangledHashes.push_back(signedBlockBatchRequestProtoHelper.getHash(index));
    }

    try {
        keto::proto::SignedBlockBatchMessage signedBlockBatchMessage;
        signedBlockBatchMessage.set_partial_result(false);
        if (!keto::block_db::BlockChainStore::getInstance()->requestBlocks(tangledHashes,signedBlockBatchMessage)) {
            KETO_LOG_INFO << "A partial result was found and we need to force a resync";
            signedBlockBatchMessage.set_partial_result(true);
            this->forceResync();
        }
        return signedBlockBatchMessage;
    } catch (...) {
        KETO_LOG_INFO << "Unexpected exception force the resync";
        this->forceResync();
        throw;
    }
}

keto::proto::MessageWrapperResponse  BlockSyncManager::processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage) {
    keto::proto::MessageWrapperResponse response;
    bool blockWriteResponse = false;
    {
        std::unique_lock<std::mutex> uniqueLock(this->classMutex);
        blockWriteResponse = keto::block_db::BlockChainStore::getInstance()->processBlockSyncResponse(signedBlockBatchMessage,
                                                                                 BlockChainCallbackImpl());
    }
    if (blockWriteResponse && !signedBlockBatchMessage.partial_result()) {
        response.set_result("complete");
        {
            std::unique_lock<std::mutex> uniqueLock(this->classMutex);
            this->startTime = 0;
            this->status = COMPLETE;
            this->stateCondition.notify_all();
        }
        KETO_LOG_INFO << "[BlockSyncManager::processBlockSyncResponse]" << " ########################################################";
        KETO_LOG_INFO << "[BlockSyncManager::processBlockSyncResponse]" << " ######## Synchronization has now been completed ########";
        KETO_LOG_INFO << "[BlockSyncManager::processBlockSyncResponse]" << " ########################################################";
        // as this node is not enabled we will not notify our
        // peers of the fact that the synchronization has been completed.
        if (this->isEnabled()) {
            BlockProducer::getInstance()->activateWaitingBlockProducer();
            notifyPeers();
        }

    } else if (signedBlockBatchMessage.partial_result()) {
        KETO_LOG_INFO << "[BlockSyncManager::processBlockSyncResponse] Server is not in sync, will need to back off.";
        response.set_result("applied");
        scheduledDelayRetry();
    } else {
        KETO_LOG_INFO << "[BlockSyncManager::processBlockSyncResponse] finished applying a block need to trigger the next request.";
        response.set_result("applied");
        {
            std::unique_lock<std::mutex> uniqueLock(this->classMutex);
            this->startTime = 0;
            this->stateCondition.notify_all();
        }
    }
    response.set_success(true);

    return response;

}

void
BlockSyncManager::processRequestBlockSyncRetry() {
    //KETO_LOG_DEBUG << "[BlockSyncManager::processRequestBlockSyncRetry] trigger the retry by resetting the start time";
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    // reschedule to run in a minutes time
    this->startTime = time(0);
    this->stateCondition.notify_all();
}

void
BlockSyncManager::scheduledDelayRetry() {
    //KETO_LOG_DEBUG << "[BlockSyncManager::processRequestBlockSyncRetry] trigger the retry by resetting the start time";
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    // reschedule to run in a minutes time
    this->startTime = time(0) - (Constants::SYNC_EXPIRY_TIME - Constants::SYNC_RETRY_DELAY_MIN);
}

void
BlockSyncManager::notifyPeers(Status status) {
    // update the status
    {
        std::unique_lock<std::mutex> uniqueLock(this->classMutex);
        this->startTime = 0;
        this->status = status;
        this->stateCondition.notify_all();
    }
    // as this node is not enabled we will not notify our
    // peers of the fact that the synchronization has been completed.
    if (this->isEnabled()) {
        notifyPeers();
    }
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
    KETO_LOG_INFO << "[BlockSyncManager::notifyPeers]" << " ########################################################";
    KETO_LOG_INFO << "[BlockSyncManager::notifyPeers]" << " ######## Notify peers of status                 ########";
    KETO_LOG_INFO << "[BlockSyncManager::notifyPeers]" << " ########################################################";
}

bool
BlockSyncManager::isEnabled() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->enabled;
}

bool
BlockSyncManager::requestFromServer() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (this->lastServerBlockTimestamp != 0 &&
        this->lastServerBlockTimestamp > this->lastClientBlockTimestamp) {
        return true;
    }
    return false;
}

void
BlockSyncManager::forceResync() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (this->status == INIT) {
        KETO_LOG_INFO << "[BlockSyncManager::forceResync] Sync is already in process ignore request to force it again" << this->status;
        return;
    }
    KETO_LOG_INFO << "[BlockSyncManager::forceResync] force the resync : " << this->status;
    this->status = INIT;
    this->startTime = 0;
    this->stateCondition.notify_all();
}

void
BlockSyncManager::forceResync(std::time_t timestamp) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (this->status == INIT) {
        KETO_LOG_INFO << "[BlockSyncManager::forceResync] Sync is already in process ignore request to force it again" << this->status;
        return;
    }
    KETO_LOG_INFO << "[BlockSyncManager::forceResync] force the resync : " << this->status;
    this->status = INIT;
    this->startTime = 0;
    this->stateCondition.notify_all();
    this->lastClientBlockTimestamp = timestamp;
}

void
BlockSyncManager::forceResyncServer(std::time_t timestamp) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (this->status == INIT) {
        KETO_LOG_INFO << "[BlockSyncManager::forceResync] Sync is already in process ignore request to force it again" << this->status;
        return;
    }
    KETO_LOG_INFO << "[BlockSyncManager::forceResync] force the resync : " << this->status;
    this->status = INIT;
    this->startTime = 0;
    this->stateCondition.notify_all();
    this->lastServerBlockTimestamp = timestamp;
}

void BlockSyncManager::broadcastBlock(const keto::block_db::SignedBlockWrapperMessageProtoHelper& signedBlockWrapperMessageProtoHelper) {
    keto::block_db::SignedBlockWrapperMessageProtoHelper _signedBlockWrapperMessageProtoHelper =
            signedBlockWrapperMessageProtoHelper;
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
            keto::server_common::Events::RPC_SERVER_BLOCK,_signedBlockWrapperMessageProtoHelper));
    KETO_LOG_INFO << "[BlockSyncManager::broadcastBlock] push block via the client : "
                << signedBlockWrapperMessageProtoHelper.getMessageHash().getHash(keto::common::StringEncoding::HEX);
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
            keto::server_common::Events::RPC_CLIENT_BLOCK,_signedBlockWrapperMessageProtoHelper));
    KETO_LOG_INFO << "[BlockSyncManager::broadcastBlock] push block via the server : "
                   << signedBlockWrapperMessageProtoHelper.getMessageHash().getHash(keto::common::StringEncoding::HEX);
}


keto::event::Event BlockSyncManager::isBlockSyncComplete(const keto::event::Event& event) {
    keto::proto::IsBlockSyncComplete isBlockSyncCompleteProto =
            keto::server_common::fromEvent<keto::proto::IsBlockSyncComplete>(event);
    Status status = getStatus();
    if (status == COMPLETE) {
        isBlockSyncCompleteProto.set_completed(true);
    } else {
        isBlockSyncCompleteProto.set_completed(false);
    }
    return keto::server_common::toEvent<keto::proto::IsBlockSyncComplete>(isBlockSyncCompleteProto);
}

bool BlockSyncManager::waitForExpired() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    std::time_t now = 0;
    std::time_t calculatedExpiryTime = this->startTime + Constants::SYNC_EXPIRY_TIME;

    // check the expiry
    while (BlockProducer::getInstance()->getState() != BlockProducer::State::terminated && (now = time(0)) < calculatedExpiryTime) {

        KETO_LOG_INFO << "[BlockSyncManager::waitForExpired] Wait for expiry [" << calculatedExpiryTime << "][" <<
        now << "] difference [" << calculatedExpiryTime - now << "]";
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(calculatedExpiryTime - now));
    }
    return true;
}



}
}