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
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->status;
}


std::time_t BlockSyncManager::getStartTime() {
    return this->startTime;
}

void BlockSyncManager::sync() {
    Status status = getStatus();
    KETO_LOG_INFO << "[BlockSyncManager::sync] start of the sync : " << status;
    if (!isExpired()) {
        KETO_LOG_INFO << "[BlockSyncManager::sync] the timeout has not been reached yet [" << this->getStartTime() << "][" <<
            time(0) << "]";
        return;
    } else if (status == COMPLETE) {
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
        KETO_LOG_INFO << "[BlockSyncManager::sync] make the block sync request";
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockBatchRequest>(
                keto::server_common::Events::RPC_CLIENT_REQUEST_BLOCK_SYNC, signedBlockBatchRequestProtoHelper));

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
    processRequestBlockSyncRetry();
}

keto::proto::SignedBlockBatchMessage  BlockSyncManager::requestBlocks(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest) {
    //std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (!keto::server_common::ServerInfo::getInstance()->isMaster() && this->getStatus() != COMPLETE) {
        KETO_LOG_DEBUG << "[BlockSyncManager::requestBlocks] This node is not synced and cannot return blocks";
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
        return keto::block_db::BlockChainStore::getInstance()->requestBlocks(tangledHashes);
    } catch (...) {
        this->forceResync();
        throw;
    }
}

keto::proto::MessageWrapperResponse  BlockSyncManager::processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage) {
    keto::proto::MessageWrapperResponse response;
    if (keto::block_db::BlockChainStore::getInstance()->processBlockSyncResponse(signedBlockBatchMessage,BlockChainCallbackImpl())) {
        response.set_result("complete");
        {
            std::unique_lock<std::mutex> uniqueLock(this->classMutex);
            this->startTime = 0;
            this->status = COMPLETE;

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

    } else {
        KETO_LOG_INFO << "[BlockSyncManager::processBlockSyncResponse] finished applying a block need to trigger the next request.";
        response.set_result("applied");
        {
            std::unique_lock<std::mutex> uniqueLock(this->classMutex);
            this->startTime = 0;
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
    this->startTime = time(0) - Constants::SYNC_RETRY_DELAY_MIN;
}

void
BlockSyncManager::notifyPeers(Status status) {
    // update the status
    {
        std::unique_lock<std::mutex> uniqueLock(this->classMutex);
        this->startTime = 0;
        this->status = status;

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

void
BlockSyncManager::forceResync() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_INFO << "[BlockSyncManager::forceResync] force the resync : " << this->status;
    this->status = INIT;
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

bool BlockSyncManager::isExpired() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    std::time_t now = time(0);
    return now > (this->startTime + Constants::SYNC_EXPIRY_TIME);
}



}
}