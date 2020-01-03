/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockService.cpp
 * Author: ubuntu
 * 
 * Created on March 8, 2018, 3:04 AM
 */

#include <condition_variable>

#include <iostream>

#include "Protocol.pb.h"
#include "BlockChain.pb.h"

#include "keto/block/BlockChainCallbackImpl.hpp"
#include "keto/block/BlockService.hpp"
#include "keto/block_db/BlockChainStore.hpp"
#include "keto/block_db/SignedBlockWrapperMessageProtoHelper.hpp"
#include "keto/block_db/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"
#include "keto/block/Constants.hpp"
#include "keto/block/GenesisReader.hpp"
#include "keto/block/GenesisLoader.hpp"
#include "keto/block/BlockSyncManager.hpp"
#include "keto/block/GenesisLoader.hpp"
#include "keto/block/Exception.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/key_store_utils/Events.hpp"

#include "keto/block/TransactionProcessor.hpp"
#include "keto/block/BlockProducer.hpp"
#include "keto/block/Constants.hpp"

#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

#include "keto/server_common/StatePersistanceManager.hpp"

#include "keto/chain_query_common/BlockQueryProtoHelper.hpp"
#include "keto/chain_query_common/BlockResultSetProtoHelper.hpp"
#include "keto/chain_query_common/TransactionQueryProtoHelper.hpp"
#include "keto/chain_query_common/TransactionResultSetProtoHelper.hpp"

namespace keto {
namespace block {

static std::shared_ptr<BlockService> singleton;

std::string BlockService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockService::SignedBlockWrapperCache::SignedBlockWrapperCache() {

}

BlockService::SignedBlockWrapperCache::~SignedBlockWrapperCache() {

}

bool BlockService::SignedBlockWrapperCache::checkCache(const keto::asn1::HashHelper& signedBlockWrapperCacheHash) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->cacheLookup.count(signedBlockWrapperCacheHash)) {
        KETO_LOG_DEBUG << "[SignedBlockWrapperCache::checkCache] The cache contains the block : " << signedBlockWrapperCacheHash.getHash(keto::common::StringEncoding::HEX);
        return true;
    }
    if (this->cacheHistory.size() >= Constants::MAX_SIGNED_BLOCK_WRAPPER_CACHE_SIZE) {
        this->cacheLookup.erase(this->cacheHistory.front());
        this->cacheHistory.pop_front();
    }
    this->cacheHistory.push_back(signedBlockWrapperCacheHash);
    this->cacheLookup.insert(signedBlockWrapperCacheHash);
    KETO_LOG_DEBUG << "[SignedBlockWrapperCache::checkCache] Adding the block to the cache : " << signedBlockWrapperCacheHash.getHash(keto::common::StringEncoding::HEX);
    return false;
}


BlockService::BlockService() {
    BlockSyncManager::createInstance(BlockProducer::getInstance()->isEnabled());

    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();

    if (config->getVariablesMap().count(Constants::STATE_STORAGE_CONFIG)) {
        keto::server_common::StatePersistanceManager::init(config->getVariablesMap()[Constants::STATE_STORAGE_CONFIG].as<std::string>());
    } else {
        keto::server_common::StatePersistanceManager::init(Constants::STATE_STORAGE_DEFAULT);
    }

    this->signedBlockWrapperCachePtr = BlockService::SignedBlockWrapperCachePtr(new SignedBlockWrapperCache());
}

BlockService::~BlockService() {
    keto::server_common::StatePersistanceManager::fin();
    BlockSyncManager::finInstance();
}

std::shared_ptr<BlockService> BlockService::init() {

    return singleton = std::make_shared<BlockService>();
}

void BlockService::fin() {
    singleton.reset();
}

std::shared_ptr<BlockService> BlockService::getInstance() {
    return singleton;
}

void BlockService::genesis() {
    if (keto::block_db::BlockChainStore::getInstance()->requireGenesis()) {
        std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    
        if (!config->getVariablesMap().count(Constants::GENESIS_CONFIG)) {
            return;
        }
        // genesis configuration
        boost::filesystem::path genesisConfig =  
                keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / 
                config->getVariablesMap()[Constants::GENESIS_CONFIG].as<std::string>();
        
        if (!boost::filesystem::exists(genesisConfig)) {
            return;
        }
        GenesisReader reader(genesisConfig);
        GenesisLoader loader(reader);
        loader.load();
    }
}

void BlockService::sync() {
    BlockSyncManager::getInstance()->sync();
}

keto::event::Event BlockService::persistBlockMessage(const keto::event::Event& event) {

    try {
        if (BlockSyncManager::getInstance()->getStatus() != BlockSyncManager::COMPLETE) {
            KETO_LOG_DEBUG << "[BlockService::persistBlockMessage]" << "Block sync is not complete ignore block.";
            keto::proto::MessageWrapperResponse response;
            response.set_success(true);
            response.set_result("ignored");

            return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
        }


        keto::block_db::SignedBlockWrapperMessageProtoHelper signedBlockWrapperMessageProtoHelper(
                keto::server_common::fromEvent<keto::proto::SignedBlockWrapperMessage>(event));
        if (keto::block_db::BlockChainStore::getInstance()->writeBlock(signedBlockWrapperMessageProtoHelper,
                                                                       BlockChainCallbackImpl()) &&
            !this->signedBlockWrapperCachePtr->checkCache(signedBlockWrapperMessageProtoHelper.getMessageHash())) {
            BlockSyncManager::getInstance()->broadcastBlock(signedBlockWrapperMessageProtoHelper);
            if (signedBlockWrapperMessageProtoHelper.getProducerEnding()) {
                BlockProducer::getInstance()->processProducerEnding(signedBlockWrapperMessageProtoHelper);
            }
        }

        keto::proto::MessageWrapperResponse response;
        response.set_success(true);
        response.set_result("persisted");

        return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
    } catch (keto::block_db::ParentHashIdentifierNotFoundException& ex) {
        KETO_LOG_ERROR << "[BlockService::persistBlockMessage] failed to persist the block : " << ex.what();
        KETO_LOG_ERROR << "[BlockService::persistBlockMessage] cause: " << boost::diagnostic_information(ex, true);
        BlockSyncManager::getInstance()->forceResync();
        keto::proto::MessageWrapperResponse response;
        response.set_success(false);
        response.set_result("out of sync");

        return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
    }
}

keto::event::Event BlockService::blockMessage(const keto::event::Event& event) {
    // aquire a transaction lock
    BlockProducer::ProducerScopeLockPtr producerScopeLockPtr =  BlockProducer::getInstance()->aquireTransactionLock();
    keto::proto::MessageWrapper  _messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    if (BlockProducer::getInstance()->getState() != BlockProducer::State::block_producer) {
        // to prevent any chance of a deadlock that might occur to do this thread looping round on itself here
        // we release the producer stop lock before making this call
        BOOST_THROW_EXCEPTION(keto::block::ReRouteMessageException());
    }

    //KETO_LOG_DEBUG << "Decrypt the transaction";
    keto::proto::MessageWrapper  decryptedMessageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MessageWrapper>(
                                    keto::key_store_utils::Events::TRANSACTION::DECRYPT_TRANSACTION,_messageWrapper)));

    //KETO_LOG_DEBUG << "###### Copy into the message wrapper";
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
            decryptedMessageWrapper);

    //KETO_LOG_DEBUG << "###### Get the proto tranction";
    keto::transaction_common::TransactionProtoHelperPtr transactionProtoHelperPtr
            = messageWrapperProtoHelper.getTransaction();

    {
        std::lock_guard<std::mutex> guard(getAccountLock(
                    transactionProtoHelperPtr->getActiveAccount()));

        //KETO_LOG_DEBUG << "###### Process the transaction";
        *transactionProtoHelperPtr =
            TransactionProcessor::getInstance()->processTransaction(
            *transactionProtoHelperPtr);
        // dirty store in the block producer

        //KETO_LOG_DEBUG << "###### add the transaction";
        BlockProducer::getInstance()->addTransaction(
            transactionProtoHelperPtr);
    }

    // release the producer scope lock to prevent recursion on this flag
    producerScopeLockPtr.reset();

    // move transaction to next phase and submit to router
    //KETO_LOG_DEBUG << "###### set the transaction";
    messageWrapperProtoHelper.setTransaction(transactionProtoHelperPtr);
    decryptedMessageWrapper = messageWrapperProtoHelper.operator keto::proto::MessageWrapper();

    //KETO_LOG_DEBUG << "###### encrypt the transaction";
    keto::proto::MessageWrapper encryptedMessageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MessageWrapper>(
                                    keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION,decryptedMessageWrapper)));

    //KETO_LOG_DEBUG << "";
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
            keto::server_common::Events::UPDATE_STATUS_ROUTE_MESSSAGE,
            encryptedMessageWrapper));
    
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    response.set_result("balanced");

    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event BlockService::requestBlockSync(const keto::event::Event& event) {
    return keto::server_common::toEvent<keto::proto::SignedBlockBatchMessage>(
            BlockSyncManager::getInstance()->requestBlocks(
            keto::server_common::fromEvent<keto::proto::SignedBlockBatchRequest>(event)));
}


keto::event::Event BlockService::processBlockSyncResponse(const keto::event::Event& event) {
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(
            BlockSyncManager::getInstance()->processBlockSyncResponse(
                    keto::server_common::fromEvent<keto::proto::SignedBlockBatchMessage>(event)));
}

keto::event::Event BlockService::processRequestBlockSyncRetry(const keto::event::Event& event) {
    BlockSyncManager::getInstance()->processRequestBlockSyncRetry();
    return event;
}

keto::event::Event BlockService::getAccountBlockTangle(const keto::event::Event& event) {
    return keto::server_common::toEvent<keto::proto::AccountChainTangle>(
            keto::block_db::BlockChainStore::getInstance()->getAccountBlockTangle(
                    keto::server_common::fromEvent<keto::proto::AccountChainTangle>(event)));
}

keto::event::Event BlockService::getBlocks(const keto::event::Event& event) {
    keto::chain_query_common::BlockQueryProtoHelper blockQueryProtoHelper(
            keto::server_common::fromEvent<keto::proto::BlockQuery>(event));

    return keto::server_common::toEvent<keto::proto::BlockResultSet>(
            *keto::block_db::BlockChainStore::getInstance()->performBlockQuery(
                    blockQueryProtoHelper));

}

keto::event::Event BlockService::getBlockTransactions(const keto::event::Event& event) {
    keto::chain_query_common::TransactionQueryProtoHelper transactionQueryProtoHelper(
            keto::server_common::fromEvent<keto::proto::TransactionQuery>(event));

    return keto::server_common::toEvent<keto::proto::TransactionResultSet>(
            *keto::block_db::BlockChainStore::getInstance()->performTransactionQuery(
                    transactionQueryProtoHelper));
}

keto::event::Event BlockService::getTransaction(const keto::event::Event& event) {
    keto::chain_query_common::TransactionQueryProtoHelper transactionQueryProtoHelper(
            keto::server_common::fromEvent<keto::proto::TransactionQuery>(event));

    return keto::server_common::toEvent<keto::proto::TransactionResultSet>(
            *keto::block_db::BlockChainStore::getInstance()->performTransactionQuery(
                    transactionQueryProtoHelper));
}

keto::event::Event BlockService::getAccountTransactions(const keto::event::Event& event) {
    keto::chain_query_common::TransactionQueryProtoHelper transactionQueryProtoHelper(
            keto::server_common::fromEvent<keto::proto::TransactionQuery>(event));

    return keto::server_common::toEvent<keto::proto::TransactionResultSet>(
            *keto::block_db::BlockChainStore::getInstance()->performTransactionQuery(
                    transactionQueryProtoHelper));
}

std::mutex& BlockService::getAccountLock(const AccountHashVector& accountHash) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (!accountLocks.count(accountHash)) {
        accountLocks[accountHash];
    }
    return accountLocks[accountHash];
}


}
}
