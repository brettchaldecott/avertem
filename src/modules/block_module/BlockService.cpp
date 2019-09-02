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

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"
#include "keto/block/Constants.hpp"
#include "keto/block/GenesisReader.hpp"
#include "keto/block/GenesisLoader.hpp"
#include "keto/block/BlockSyncManager.hpp"
#include "include/keto/block/GenesisLoader.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/key_store_utils/Events.hpp"

#include "keto/block/TransactionProcessor.hpp"
#include "keto/block/BlockProducer.hpp"
#include "keto/block/Constants.hpp"

#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

#include "keto/server_common/StatePersistanceManager.hpp"

namespace keto {
namespace block {

static std::shared_ptr<BlockService> singleton;

std::string BlockService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockService::BlockService() {
    BlockSyncManager::createInstance();

    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();

    if (config->getVariablesMap().count(Constants::STATE_STORAGE_CONFIG)) {
        keto::server_common::StatePersistanceManager::init(config->getVariablesMap()[Constants::STATE_STORAGE_CONFIG].as<std::string>());
    } else {
        keto::server_common::StatePersistanceManager::init(Constants::STATE_STORAGE_DEFAULT);
    }
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
    if (BlockSyncManager::getInstance()->getStatus() != BlockSyncManager::COMPLETE) {
        KETO_LOG_DEBUG << "[BlockService::persistBlockMessage]" << "Block sync is not complete ignore block.";
        keto::proto::MessageWrapperResponse response;
        response.set_success(true);
        response.set_result("ignored");

        return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
    }

    keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage =
            keto::server_common::fromEvent<keto::proto::SignedBlockWrapperMessage>(event);
    keto::block_db::BlockChainStore::getInstance()->writeBlock(signedBlockWrapperMessage,BlockChainCallbackImpl());


    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    response.set_result("persisted");

    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event BlockService::blockMessage(const keto::event::Event& event) {
    keto::proto::MessageWrapper  _messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);

    //std::cout << "Decrypt the transaction" << std::endl;
    keto::proto::MessageWrapper  decryptedMessageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MessageWrapper>(
                                    keto::key_store_utils::Events::TRANSACTION::DECRYPT_TRANSACTION,_messageWrapper)));

    //std::cout << "###### Copy into the message wrapper" << std::endl;
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
            decryptedMessageWrapper);

    //std::cout << "###### Get the proto tranction" << std::endl;
    keto::transaction_common::TransactionProtoHelperPtr transactionProtoHelperPtr
            = messageWrapperProtoHelper.getTransaction();

    {
        std::lock_guard<std::mutex> guard(getAccountLock(
                    transactionProtoHelperPtr->getActiveAccount()));

        //std::cout << "###### Process the transaction" << std::endl;
        *transactionProtoHelperPtr =
            TransactionProcessor::getInstance()->processTransaction(
            *transactionProtoHelperPtr);
        // dirty store in the block producer

        //std::cout << "###### add the transaction" << std::endl;
        BlockProducer::getInstance()->addTransaction(
            transactionProtoHelperPtr);
    }



    // move transaction to next phase and submit to router
    //std::cout << "###### set the transaction" << std::endl;
    messageWrapperProtoHelper.setTransaction(transactionProtoHelperPtr);
    decryptedMessageWrapper = messageWrapperProtoHelper.operator keto::proto::MessageWrapper();

    //std::cout << "###### encrypt the transaction" << std::endl;
    keto::proto::MessageWrapper encryptedMessageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MessageWrapper>(
                                    keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION,decryptedMessageWrapper)));

    //std::cout << "" << std::endl;
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

std::mutex& BlockService::getAccountLock(const AccountHashVector& accountHash) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (!accountLocks.count(accountHash)) {
        accountLocks[accountHash];
    }
    return accountLocks[accountHash];
}


}
}