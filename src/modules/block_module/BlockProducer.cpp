/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockProducer.cpp
 * Author: ubuntu
 * 
 * Created on April 2, 2018, 10:38 AM
 */

#include <iostream>

#include "keto/block/BlockProducer.hpp"

#include "keto/common/Log.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/block_db/BlockBuilder.hpp"
#include "keto/block_db/SignedBlockBuilder.hpp"
#include "keto/block_db/BlockChainStore.hpp"

#include "keto/block/Constants.hpp"

#include "include/keto/block/BlockProducer.hpp"
#include "include/keto/block/Exception.hpp"

#include "keto/transaction/Transaction.hpp"
#include "keto/server_common/TransactionHelper.hpp"

#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/ChangeSetBuilder.hpp"
#include "keto/transaction_common/SignedChangeSetBuilder.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"


#include "keto/chain_common/ActionBuilder.hpp"
#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"


namespace keto {
namespace block {

static BlockProducerPtr singleton;
static std::shared_ptr<std::thread> producerThreadPtr; 

std::string BlockProducer::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockProducer::BlockProducer() : 
        enabled(false),
        currentState(State::inited) {
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::block::PrivateKeyNotConfiguredException());
    }
    std::string privateKeyPath = 
            config->getVariablesMap()[Constants::PRIVATE_KEY].as<std::string>();
    if (!config->getVariablesMap().count(Constants::PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::block::PublicKeyNotConfiguredException());
    }
    std::string publicKeyPath = 
            config->getVariablesMap()[Constants::PUBLIC_KEY].as<std::string>();
    keyLoaderPtr = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
            publicKeyPath);
    
    if (config->getVariablesMap().count(Constants::BLOCK_PRODUCER_ENABLED)) {
        this->enabled = 
                config->getVariablesMap()[Constants::BLOCK_PRODUCER_ENABLED].as<std::string>().compare(
                Constants::BLOCK_PRODUCER_ENABLED_TRUE) == 0;
    }
}

BlockProducer::~BlockProducer() {
}

BlockProducerPtr BlockProducer::init() {
    singleton = std::make_shared<BlockProducer>();
    if (singleton->isEnabled()) {
    producerThreadPtr = std::shared_ptr<std::thread>(new std::thread(
        []
        {
            singleton->run();
        }));
    }
    return singleton;
}

void BlockProducer::fin() {
    if (singleton->isEnabled()) {
        singleton->terminate();
        producerThreadPtr->join();
        producerThreadPtr.reset();
    }
    singleton.reset();
}

BlockProducerPtr BlockProducer::getInstance() {
    return singleton;
}

void BlockProducer::run() {
    BlockProducer::State currentState;
    while((currentState = this->checkState()) != State::terminated) {
        if (currentState == BlockProducer::State::block_producer) {
            generateBlock(this->getTransactions());
        }
    }
}

void BlockProducer::terminate() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->currentState = State::terminated;
    this->stateCondition.notify_all();
}

void BlockProducer::setState(const State& state) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (currentState == State::terminated || state == State::terminated) {
        return;
    }
    // the block producer has to be enabled.
    if (!this->enabled && state == State::block_producer) {
        return;
    }
    this->currentState = state;
    this->stateCondition.notify_all();
}


keto::event::Event BlockProducer::setupNodeConsensusSession(const keto::event::Event& event) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    this->consensusMessageHelper = keto::software_consensus::ConsensusMessageHelper(
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(event));
    return event;
}

BlockProducer::State BlockProducer::getState() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return this->currentState;
}

void BlockProducer::addTransaction(keto::proto::Transaction transaction) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (this->currentState == State::terminated) {
        BOOST_THROW_EXCEPTION(keto::block::BlockProducerTerminatedException());
    }
    this->transactions.push_back(transaction);
}

bool BlockProducer::isEnabled() {
    return enabled;
}

BlockProducer::State BlockProducer::checkState() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_DEBUG << "[BlockProducer] wait";
    this->stateCondition.wait_for(uniqueLock,std::chrono::milliseconds(20 * 1000));
    KETO_LOG_DEBUG << "[Block Producer] run";
    return this->currentState;
}

std::deque<keto::proto::Transaction> BlockProducer::getTransactions() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    std::deque<keto::proto::Transaction> transactions = this->transactions;
    this->transactions.clear();
    return transactions;
}


void BlockProducer::generateBlock(std::deque<keto::proto::Transaction> transactions) {
    // create a new transaction
    keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
    
    keto::asn1::HashHelper parentHash = keto::block_db::BlockChainStore::getInstance()->getParentHash();
    keto::block_db::BlockBuilderPtr blockBuilderPtr = 
            std::make_shared<keto::block_db::BlockBuilder>(parentHash);
    for (keto::proto::Transaction& transaction : transactions) {
        std::cout << "The transaction" << std::endl;
        keto::transaction_common::TransactionProtoHelper transactionProtoHelper(
            keto::server_common::fromEvent<keto::proto::Transaction>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::Transaction>(
            keto::server_common::Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE,transaction))));
        
        blockBuilderPtr->addTransactionMessage(transactionProtoHelper.getTransactionMessageHelper());
    }
    keto::block_db::SignedBlockBuilderPtr signedBlockBuilderPtr(new keto::block_db::SignedBlockBuilder(
            blockBuilderPtr->operator Block_t*(),
            keyLoaderPtr));
    signedBlockBuilderPtr->sign();
    
    KETO_LOG_INFO << "Write a block";
    keto::block_db::BlockChainStore::getInstance()->writeBlock(*signedBlockBuilderPtr);
    KETO_LOG_INFO << "Wrote a block [" << 
            keto::block_db::BlockChainStore::getInstance()->getBlockCount() << "][" <<
            keto::block_db::BlockChainStore::getInstance()->getParentHash().getHash(keto::common::HEX)
            << "]";
    
    transactionPtr->commit();
    
}

}
}