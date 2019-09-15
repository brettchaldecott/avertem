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
#include <chrono>
#include <thread>

#include "Account.pb.h"

#include <keto/block_db/MerkleUtils.hpp>

#include "keto/block/BlockProducer.hpp"
#include "keto/block/BlockSyncManager.hpp"

#include "keto/common/Log.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/block_db/BlockBuilder.hpp"
#include "keto/block_db/SignedBlockBuilder.hpp"
#include "keto/block_db/BlockChainStore.hpp"
#include "keto/block_db/MerkleUtils.hpp"

#include "keto/block/Constants.hpp"

#include "include/keto/block/BlockProducer.hpp"
#include "include/keto/block/Exception.hpp"
#include "include/keto/block/BlockChainCallbackImpl.hpp"

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
#include "keto/server_common/StatePersistanceManager.hpp"


#include "keto/software_consensus/ConsensusStateManager.hpp"
#include "keto/software_consensus/SoftwareConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"
#include "keto/software_consensus/ProtocolHeartbeatMessageHelper.hpp"

#include "keto/block/StorageManager.hpp"
#include "keto/block/BlockService.hpp"
#include "keto/block/ElectionManager.hpp"

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"

namespace keto {
namespace block {

static BlockProducerPtr singleton;
static std::shared_ptr<std::thread> producerThreadPtr;

std::string BlockProducer::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockProducer::TangleFutureStateManager::TangleFutureStateManager(const keto::asn1::HashHelper& tangleHash, bool existing) :
    tangleHash(tangleHash), existing(existing), numberOfAccounts(0) {
    if (existing) {
        keto::block_db::BlockChainTangleMetaPtr blockChainTangleMetaPtr =
                keto::block_db::BlockChainStore::getInstance()->getTangleInfo(tangleHash);
        this->lastBlockHash = blockChainTangleMetaPtr->getLastBlockHash();
        this->numberOfAccounts = blockChainTangleMetaPtr->getNumberOfAccounts();
    } else {
        this->lastBlockHash = tangleHash;
    }
}

BlockProducer::TangleFutureStateManager::~TangleFutureStateManager() {
}

bool BlockProducer::TangleFutureStateManager::isExisting() {
    return this->existing;
}

keto::asn1::HashHelper BlockProducer::TangleFutureStateManager::getTangleHash() {
    return this->tangleHash;
}

keto::asn1::HashHelper BlockProducer::TangleFutureStateManager::getLastBlockHash() {
    if (keto::block_db::BlockChainStore::getInstance()->containsTangleInfo(this->tangleHash)) {
        keto::block_db::BlockChainTangleMetaPtr blockChainTangleMetaPtr =
                keto::block_db::BlockChainStore::getInstance()->getTangleInfo(tangleHash);
        this->lastBlockHash = blockChainTangleMetaPtr->getLastBlockHash();
    }
    return this->lastBlockHash;
}

int BlockProducer::TangleFutureStateManager::getNumerOfAccounts() {
    return this->numberOfAccounts;
}

int BlockProducer::TangleFutureStateManager::incrementNumberOfAccounts() {
    return this->numberOfAccounts++;
}

BlockProducer::PendingTransactionsTangle::PendingTransactionsTangle(const keto::asn1::HashHelper& tangleHash, bool existing) {
    this->tangleFutureStateManagerPtr =
            BlockProducer::TangleFutureStateManagerPtr(new BlockProducer::TangleFutureStateManager(tangleHash,existing));

}

BlockProducer::PendingTransactionsTangle::~PendingTransactionsTangle() {
}


BlockProducer::TangleFutureStateManagerPtr BlockProducer::PendingTransactionsTangle::getTangle() {
    return this->tangleFutureStateManagerPtr;
}

void BlockProducer::PendingTransactionsTangle::addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr) {
    this->transactions.push_back(transactionProtoHelperPtr);
}

std::deque<keto::transaction_common::TransactionProtoHelperPtr> BlockProducer::PendingTransactionsTangle::getTransactions() {
    return this->transactions;
}

std::deque<keto::transaction_common::TransactionProtoHelperPtr> BlockProducer::PendingTransactionsTangle::takeTransactions() {
    std::deque<keto::transaction_common::TransactionProtoHelperPtr> transactions(this->transactions);
    this->transactions.clear();
    return transactions;
}

bool BlockProducer::PendingTransactionsTangle::empty() {
    return !this->transactions.size();
}

BlockProducer::PendingTransactionManager::PendingTransactionManager() : _empty(true) {

}

BlockProducer::PendingTransactionManager::~PendingTransactionManager() {

}


void BlockProducer::PendingTransactionManager::addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    keto::asn1::HashHelper tangleHash;
    BlockProducer::PendingTransactionsTanglePtr pendingTransactionsTanglePtr;
    if (keto::block_db::BlockChainStore::getInstance()->getAccountTangle(
            transactionProtoHelperPtr->getTransactionMessageHelper()->getTransactionWrapper()->getCurrentAccount(),tangleHash)) {
        pendingTransactionsTanglePtr = getPendingTransactionTangle(tangleHash);
    } else {
        pendingTransactionsTanglePtr = getGrowingPendingTransactionTangle();
    }
    pendingTransactionsTanglePtr->addTransaction(transactionProtoHelperPtr);
    this->_empty = false;
}

std::deque<BlockProducer::PendingTransactionsTanglePtr> BlockProducer::PendingTransactionManager::takeTransactions() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    std::deque<BlockProducer::PendingTransactionsTanglePtr> result(this->pendingTransactions);
    this->_empty = true;
    return result;
}

bool BlockProducer::PendingTransactionManager::empty() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return this->_empty;
}

void BlockProducer::PendingTransactionManager::clear() {
    this->pendingTransactions.clear();
    this->growTanglePtr.reset();
    this->tangleTransactions.clear();
}

BlockProducer::PendingTransactionsTanglePtr BlockProducer::PendingTransactionManager::getPendingTransactionTangle(
        const keto::asn1::HashHelper& tangleHash, bool existing) {
    if (!this->tangleTransactions.count(tangleHash)) {
        BlockProducer::PendingTransactionsTanglePtr pendingTransactionsTanglePtr(new BlockProducer::PendingTransactionsTangle(tangleHash,existing));
        this->tangleTransactions.insert(std::pair<std::vector<uint8_t>,PendingTransactionsTanglePtr>(tangleHash,
                                                                                                     pendingTransactionsTanglePtr));
        this->pendingTransactions.push_back(pendingTransactionsTanglePtr);
    }
    return this->tangleTransactions[tangleHash];
}

BlockProducer::PendingTransactionsTanglePtr BlockProducer::PendingTransactionManager::getGrowingPendingTransactionTangle() {
    if (this->growTanglePtr && !this->growTanglePtr->getTangle()->isExisting()) {
        return this->growTanglePtr;
    } else if (this->growTanglePtr && this->growTanglePtr->getTangle()->getNumerOfAccounts() < Constants::MAX_TANGLE_ACCOUNTS) {
        this->growTanglePtr->getTangle()->incrementNumberOfAccounts();
        return this->growTanglePtr;
    } else if (this->growTanglePtr) {
        BlockProducer::PendingTransactionsTanglePtr pendingTransactionsTanglePtr(new BlockProducer::PendingTransactionsTangle(
                this->growTanglePtr->getTangle()->getLastBlockHash(),false));
        this->tangleTransactions.insert(std::pair<std::vector<uint8_t>,PendingTransactionsTanglePtr>(
                this->growTanglePtr->getTangle()->getLastBlockHash(),
                pendingTransactionsTanglePtr));
        this->pendingTransactions.push_back(pendingTransactionsTanglePtr);
        return this->growTanglePtr = pendingTransactionsTanglePtr;
    } else {
        BlockProducer::PendingTransactionsTanglePtr pendingTransactionsTanglePtr(new BlockProducer::PendingTransactionsTangle(
                keto::block_db::BlockChainStore::getInstance()->getGrowTangle(),true));
        this->tangleTransactions.insert(std::pair<std::vector<uint8_t>,PendingTransactionsTanglePtr>(
                pendingTransactionsTanglePtr->getTangle()->getTangleHash(),
                pendingTransactionsTanglePtr));
        this->pendingTransactions.push_back(pendingTransactionsTanglePtr);
        return this->growTanglePtr = pendingTransactionsTanglePtr;
    }
}

BlockProducer::BlockProducer() : 
        enabled(false),
        loaded(false),
        delay(0),
        currentState(State::unloaded),
        safe(true){
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

    if (config->getVariablesMap().count(Constants::BLOCK_PRODUCER_SAFE_MODE)) {
        this->safe =
                config->getVariablesMap()[Constants::BLOCK_PRODUCER_SAFE_MODE].as<std::string>().compare(
                        Constants::BLOCK_PRODUCER_SAFE_MODE_ENABLED_TRUE) != 0;
    }

    this->pendingTransactionManagerPtr =
            BlockProducer::PendingTransactionManagerPtr(new BlockProducer::PendingTransactionManager());
}

BlockProducer::~BlockProducer() {
    ElectionManager::fin();
}

BlockProducerPtr BlockProducer::init() {
    singleton = std::make_shared<BlockProducer>();
    // enable the block producer thread this is required for sync and for production
    producerThreadPtr = std::shared_ptr<std::thread>(new std::thread(
        []
        {
            singleton->run();
        }));
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
    // load the block chain first as it will need to be initialized before syncronization or processing can begin.
    while(!isLoaded() && this->checkState() != State::terminated) {
        // load the chain only after we have been accepted to the network
        // it will not be possible to load it until then
        if (keto::software_consensus::ConsensusStateManager::getInstance()->getState()
            == keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
            load();
        }
    }

    // process the
    BlockProducer::State currentState;
    while((currentState = this->checkState()) != State::terminated) {
        if (currentState == BlockProducer::State::block_producer &&
        keto::software_consensus::ConsensusStateManager::getInstance()->getState()
        == keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
            processTransactions();
        } else if (currentState == BlockProducer::State::sync_blocks &&
                   keto::software_consensus::ConsensusStateManager::getInstance()->getState()
                   == keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
            sync();
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
    _setState(state);
}

void BlockProducer::loadState(const State& state) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (this->currentState == BlockProducer::State::unloaded) {
        keto::server_common::StatePersistanceManagerPtr statePersistanceManagerPtr =
                keto::server_common::StatePersistanceManager::getInstance();
        if (statePersistanceManagerPtr->contains(Constants::PERSISTED_STATE)) {
            this->currentState = (BlockProducer::State)(long)(*statePersistanceManagerPtr)[Constants::PERSISTED_STATE];
        } else {
            this->currentState = BlockProducer::State::inited;
            _setState(state);
        }
    } else {
        _setState(state);
    }
}


BlockProducer::State BlockProducer::getState() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return this->currentState;
}

bool BlockProducer::isSafe() {
    return this->safe;
}


void BlockProducer::_setState(const State& state) {
    keto::server_common::StatePersistanceManagerPtr statePersistanceManagerPtr =
            keto::server_common::StatePersistanceManager::getInstance();
    keto::server_common::StatePersistanceManager::StateMonitorPtr stateMonitorPtr =
            statePersistanceManagerPtr->createStateMonitor();
    if (currentState == State::terminated || state == State::terminated) {
        return;
    }
    // the block producer has to be enabled.
    if (!this->enabled && state == State::block_producer) {
        KETO_LOG_DEBUG << "[ElectionManager::_setState] the block producer has not been enabled, cannot be a producer";
        return;
    }
    // the block producer has to be enabled.
    if (this->enabled && state == State::block_producer &&
        keto::software_consensus::ConsensusStateManager::getInstance()->getState()
        != keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
        BOOST_THROW_EXCEPTION(keto::block::BlockProducerNotAcceptedByNetworkException());
    }

    KETO_LOG_DEBUG << "[ElectionManager::_setState] set the state to : " << state;
    this->currentState = state;
    if (currentState != State::terminated) {
        (*statePersistanceManagerPtr)[Constants::PERSISTED_STATE].set((long) this->currentState);
    }

    this->stateCondition.notify_all();
}

BlockProducer::State BlockProducer::_getState() {
    return this->currentState;
}

keto::event::Event BlockProducer::setupNodeConsensusSession(const keto::event::Event& event) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    this->consensusMessageHelper = keto::software_consensus::ConsensusMessageHelper(
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(event));
    return event;
}

void BlockProducer::addTransaction(keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (this->currentState == State::terminated) {
        BOOST_THROW_EXCEPTION(keto::block::BlockProducerTerminatedException());
    }
    if (this->currentState != State::block_producer) {
        BOOST_THROW_EXCEPTION(keto::block::NotBlockProducerException());
    }
    // apply the transaction dirty so that sequential transactions on this account can get the data before a block closes.
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =transactionProtoHelperPtr->getTransactionMessageHelper();
    keto::block_db::BlockChainStore::getInstance()->applyDirtyTransaction(transactionMessageHelperPtr,
            BlockChainCallbackImpl());

    // add a processed transaction
    this->pendingTransactionManagerPtr->addTransaction(transactionProtoHelperPtr);
}

bool BlockProducer::isEnabled() {
    return this->enabled;
}

bool BlockProducer::isLoaded() {
    return this->loaded;
}

keto::software_consensus::ConsensusMessageHelper BlockProducer::getAcceptedCheck() {
    return this->consensusMessageHelper;
}


keto::crypto::KeyLoaderPtr BlockProducer::getKeyLoader() {
    return this->keyLoaderPtr;
}

// setup the active tangles
std::vector<keto::asn1::HashHelper> BlockProducer::getActiveTangles() {
    return keto::block_db::BlockChainStore::getInstance()->getActiveTangles();
}

void BlockProducer::setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    keto::block_db::BlockChainStore::getInstance()->setActiveTangles(tangles);
    if (tangles.size()) {
        _setState(BlockProducer::State::block_producer);
        this->delay = Constants::ACTIVATE_PRODUCER_DELAY;
    } else if (_getState() == BlockProducer::State::block_producer) {
        for(int retries = 0; (!this->pendingTransactionManagerPtr->empty()) &&
            (retries < Constants::BLOCK_PRODUCER_RETRY_MAX); retries++) {
            this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                    Constants::BLOCK_PRDUCER_DEACTIVATE_CHECK_DELAY));
        }
        this->pendingTransactionManagerPtr->clear();
        _setState(BlockProducer::State::sync_blocks);
    }
}

BlockProducer::State BlockProducer::checkState() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_DEBUG << "[BlockProducer::checkState] check the state";
    if (this->currentState == State::terminated) {
        return this->currentState;
    }
    KETO_LOG_DEBUG << "[BlockProducer::checkState] perform the delay";
    State result = this->currentState;
    if (delay) {
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(delay));
        delay = 0;
    } else {
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(Constants::BLOCK_TIME));
    }
    KETO_LOG_DEBUG << "[BlockProducer::checkState] return the state";
    return result;
}


void BlockProducer::processTransactions() {
    // clear the account cache
    keto::proto::ClearDirtyCache clearDirtyCache;
    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::ClearDirtyCache>(
                    keto::server_common::Events::CLEAR_DIRTY_CACHE,clearDirtyCache));

    for (BlockProducer::PendingTransactionsTanglePtr pendingTransactionTanglePtr : this->pendingTransactionManagerPtr->takeTransactions()) {
        keto::block_db::BlockChainStore::getInstance()->setCurrentTangle(pendingTransactionTanglePtr->getTangle()->getTangleHash());
        generateBlock(pendingTransactionTanglePtr);
    }

}


void BlockProducer::generateBlock(const BlockProducer::PendingTransactionsTanglePtr& pendingTransactionTanglePtr) {

    try {

        // create a new transaction
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();

        keto::asn1::HashHelper parentHash = pendingTransactionTanglePtr->getTangle()->getLastBlockHash();
        if (parentHash.empty()) {
            KETO_LOG_INFO << "The block producer is not initialized";
            return;
        }
        keto::block_db::BlockBuilderPtr blockBuilderPtr =
                std::make_shared<keto::block_db::BlockBuilder>(parentHash);

        for (keto::transaction_common::TransactionProtoHelperPtr transaction : pendingTransactionTanglePtr->takeTransactions()) {
            blockBuilderPtr->addTransactionMessage(transaction->getTransactionMessageHelper());
        }
        blockBuilderPtr->setAcceptedCheck(this->consensusMessageHelper.getMsg());

        // build the block consensus based on the software consensus
        keto::block_db::MerkleUtils merkleUtils(blockBuilderPtr->getCurrentHashs());
        keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper;
        keto::asn1::HashHelper consensusMerkelHash = merkleUtils.computation();
        moduleHashMessageHelper.setHash(consensusMerkelHash);
        keto::proto::ModuleHashMessage moduleHashMessage = moduleHashMessageHelper.getModuleHashMessage();
        keto::software_consensus::ConsensusMessageHelper consensusMessageHelper(
                keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                        keto::server_common::processEvent(
                                keto::server_common::toEvent<keto::proto::ModuleHashMessage>(
                                        keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,
                                        moduleHashMessage))));
        blockBuilderPtr->setValidateCheck(consensusMessageHelper.getMsg());

        keto::block_db::SignedBlockBuilderPtr signedBlockBuilderPtr(new keto::block_db::SignedBlockBuilder(
                blockBuilderPtr,
                keyLoaderPtr));
        signedBlockBuilderPtr->sign();

        KETO_LOG_INFO << "Write a block";
        keto::block_db::BlockChain::clearCache();
        keto::block_db::BlockChainStore::getInstance()->writeBlock(signedBlockBuilderPtr, BlockChainCallbackImpl());
        KETO_LOG_INFO << "Wrote a block [" <<
                      signedBlockBuilderPtr->getHash().getHash(keto::common::StringEncoding::HEX)
                      << "]";

        transactionPtr->commit();
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::generateBlock]Failed to create a new block: " << ex.what();
        KETO_LOG_ERROR << "[BlockProducer::generateBlock]Cause: " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::generateBlock]Failed to create a new block: " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::generateBlock]Failed to create a new block: " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[BlockProducer::generateBlock]Failed to create a new block: unknown cause";
    }
    
}


void BlockProducer::load() {
    if (loaded) {
        return;
    }

    try {
        // wait 20 seconds for consensus notifications to complete
        std::this_thread::sleep_for(std::chrono::seconds(20));

        // load the information
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
        StorageManager::getInstance()->load();

        // check if this is a block producer
        if (this->getState() == BlockProducer::State::block_producer) {
            BlockService::getInstance()->genesis();
            BlockSyncManager::getInstance()->notifyPeers();
        }
        transactionPtr->commit();
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::load]Failed to load: " << ex.what();
        KETO_LOG_ERROR << "[BlockProducer::load]Cause: " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::load]Failed to create a new block: " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::load]Failed to create a new block: " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[BlockProducer::load]Failed to create a new block: unknown cause";
    }

    loaded = true;
}

void BlockProducer::sync() {
    try {
        while (this->getState()  == BlockProducer::State::sync_blocks &&
            BlockSyncManager::getInstance()->getStatus() != BlockSyncManager::COMPLETE) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
            BlockService::getInstance()->sync();
            transactionPtr->commit();
        }
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::sync]Failed to sync : " << ex.what();
        KETO_LOG_ERROR << "[BlockProducer::sync]Cause : " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::sync]Failed sync : " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[BlockProducer::sync]Failed sync : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[BlockProducer::sync]Failed sync: unknown cause";
    }
}

}
}