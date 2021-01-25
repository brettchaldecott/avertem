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
#include "Route.pb.h"

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
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return this->tangleFutureStateManagerPtr;
}

void BlockProducer::PendingTransactionsTangle::addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    this->pendingTransactions.push_back(transactionProtoHelperPtr);
}

std::deque<keto::transaction_common::TransactionProtoHelperPtr> BlockProducer::PendingTransactionsTangle::getTransactions() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return this->pendingTransactions;
}

std::deque<keto::transaction_common::TransactionProtoHelperPtr> BlockProducer::PendingTransactionsTangle::takeTransactions() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    keto::server_common::enlistResource(*this);
    std::deque<keto::transaction_common::TransactionProtoHelperPtr> transactions(this->pendingTransactions);
    this->activeTransactions = transactions;
    this->pendingTransactions.clear();
    return transactions;
}

// commit and rollbak
void BlockProducer::PendingTransactionsTangle::commit() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    this->activeTransactions.clear();
}

void BlockProducer::PendingTransactionsTangle::rollback() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    std::deque<keto::transaction_common::TransactionProtoHelperPtr> transactions(this->activeTransactions);
    transactions.insert(transactions.end(),this->pendingTransactions.begin(),this->pendingTransactions.end());
    this->pendingTransactions = transactions;
    this->activeTransactions.clear();
}

bool BlockProducer::PendingTransactionsTangle::empty() {
    return !this->pendingTransactions.size();
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
    keto::server_common::enlistResource(*this);
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


// commit and rollbak
void BlockProducer::PendingTransactionManager::commit() {

}

void BlockProducer::PendingTransactionManager::rollback() {

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

BlockProducer::ProducerLock::ProducerLock() : transactionLock(0), blockLock(0) {

}

BlockProducer::ProducerLock::~ProducerLock() {

}

BlockProducer::ProducerScopeLockPtr BlockProducer::ProducerLock::aquireBlockLock() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->blockLock++;
    while(this->transactionLock){
        this->stateCondition.wait(uniqueLock);
    }
    return BlockProducer::ProducerScopeLockPtr(new BlockProducer::ProducerScopeLock(this,false,true));
}

BlockProducer::ProducerScopeLockPtr BlockProducer::ProducerLock::aquireTransactionLock() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    while(this->blockLock) {
        this->stateCondition.wait(uniqueLock);
    }
    this->transactionLock++;
    return BlockProducer::ProducerScopeLockPtr(new BlockProducer::ProducerScopeLock(this,true,false));
}

void BlockProducer::ProducerLock::release(bool _transactionLock, bool _blockLock) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_DEBUG << "[BlockProducer::ProducerLock::release]Release the lock [" << this->transactionLock << "][" <<this->blockLock << "]";
    if (_transactionLock) {
        this->transactionLock--;
    } else if (_blockLock) {
        this->blockLock--;
    }
    // use notify all this is cumbersome but a lot more effective than having aquires individually release until a lock is
    // aquired
    this->stateCondition.notify_all();
}

BlockProducer::ProducerScopeLock::ProducerScopeLock(BlockProducer::ProducerLock* reference, bool transactionLock, bool blockLock) :
    reference(reference),transactionLock(transactionLock), blockLock(blockLock) {
    KETO_LOG_DEBUG << "[BlockProducer::ProducerScopeLock::ProducerScopeLock]gain the lock the lock [" << this->transactionLock << "][" <<this->blockLock << "]";
}

BlockProducer::ProducerScopeLock::~ProducerScopeLock() {
    KETO_LOG_DEBUG << "[BlockProducer::ProducerScopeLock::~ProducerScopeLock]Release the lock [" << this->transactionLock << "][" <<this->blockLock << "]";
    reference->release(this->transactionLock,this->blockLock);
}

BlockProducer::BlockProducer() :
        enabled(false),
        loaded(false),
        delay(0),
        currentState(State::unloaded),
        producerState(ProducerState::idle),
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

    this->producerLockPtr = BlockProducer::ProducerLockPtr(new ProducerLock());
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
        try {
            if (currentState == BlockProducer::State::block_producer &&
                keto::software_consensus::ConsensusStateManager::getInstance()->getState()
                == keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
                processTransactions();
            } else if (currentState == BlockProducer::State::sync_blocks &&
                       keto::software_consensus::ConsensusStateManager::getInstance()->getState()
                       == keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
                sync();
            } else if (currentState == BlockProducer::State::block_producer_wait &&
                        BlockSyncManager::getInstance()->getStatus() != BlockSyncManager::Status::COMPLETE &&
                       keto::software_consensus::ConsensusStateManager::getInstance()->getState()
                       == keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
                // perform a sync using the block sync manager as we are very out of sync and will not be able
                // to apply blocks or write blocks until this occurs.
                waitingBlockProducerSync();
            }
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[BlockProducer::run] failed to run block producer: " << ex.what();
            KETO_LOG_ERROR << "[BlockProducer::run] cause: " << boost::diagnostic_information(ex, true);
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[BlockProducer::run] failed to run the block producer : "
                           << boost::diagnostic_information(ex, true);
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[BlockProducer::run] failed to run the block producer : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[BlockProducer::run] failed to run the block producer";
        }
    }
}

void BlockProducer::terminate() {
    {
        std::unique_lock<std::mutex> uniqueLock(this->classMutex);
        this->currentState = State::terminated;
        this->stateCondition.notify_all();
    }

    producerThreadPtr->join();
    producerThreadPtr.reset();
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
            KETO_LOG_INFO << "[BlockProducer::loadState] Use the persisted state";
            this->currentState = (BlockProducer::State)(long)(*statePersistanceManagerPtr)[Constants::PERSISTED_STATE];
        } else {
            this->currentState = BlockProducer::State::inited;
            _setState(state);
        }
        KETO_LOG_INFO << "[BlockProducer::loadState] The loaded state is now : " << this->currentState;
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
        KETO_LOG_DEBUG << "[BlockProducer::_setState] the block producer has not been enabled, cannot be a producer";
        return;
    }
    // the block producer has to be enabled.
    if (this->enabled && state == State::block_producer &&
        keto::software_consensus::ConsensusStateManager::getInstance()->getState()
        != keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
        BOOST_THROW_EXCEPTION(keto::block::BlockProducerNotAcceptedByNetworkException());
    }

    KETO_LOG_DEBUG << "[BlockProducer::_setState] set the state to : " << state;
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
    // can be a block producer but not producing at the end of a cycle.
    if (this->producerState != ProducerState::producing) {
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

void BlockProducer::clearActiveTangles() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (_getState() == BlockProducer::State::block_producer) {
        _setProducerState(BlockProducer::ProducerState::ending);
        while(_getProducerState() == BlockProducer::ProducerState::ending) {
            this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                    Constants::BLOCK_PRDUCER_DEACTIVATE_CHECK_DELAY));
        }
        _setProducerState(BlockProducer::ProducerState::idle);
        if (_getState() == BlockProducer::State::block_producer) {
            _setState(BlockProducer::State::sync_blocks);
        }
    }
    // clear the active tangles after the given delay
    keto::block_db::BlockChainStore::getInstance()->clearActiveTangles();
}

void BlockProducer::setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    BlockProducer::State state = _getState();
    // logic assumes the node selected for producing will receive this state update
    if (tangles.size()) {
        keto::block_db::BlockChainStore::getInstance()->setActiveTangles(tangles);
        // there is a chance the block producer might receive this tangle update post status change
        // if this happens then the block producer enters a state it will never leave.
        if (state != BlockProducer::State::block_producer && state != BlockProducer::State::block_producer_wait) {
            _setProducerState(BlockProducer::ProducerState::producing);
            _setState(BlockProducer::State::block_producer_wait);
            this->delay = Constants::ACTIVATE_PRODUCER_DELAY;
        }
    } else if (state == BlockProducer::State::block_producer) {
        _setProducerState(BlockProducer::ProducerState::ending);
        while(_getProducerState() == BlockProducer::ProducerState::ending) {
            this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                    Constants::BLOCK_PRDUCER_DEACTIVATE_CHECK_DELAY));
        }
        //this->pendingTransactionManagerPtr->clear();
        //_setState(BlockProducer::State::sync_blocks);
        _setProducerState(BlockProducer::ProducerState::idle);
    } else {
        keto::block_db::BlockChainStore::getInstance()->setActiveTangles(tangles);
    }

}

void BlockProducer::processProducerEnding(
        const keto::block_db::SignedBlockWrapperMessageProtoHelper& signedBlockWrapperMessageProtoHelper) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_DEBUG << "[BlockProducer::processProducerEnding] The processor ending has been set for : "
                  << signedBlockWrapperMessageProtoHelper.getMessageHash().getHash(keto::common::StringEncoding::HEX);
    if (keto::block_db::BlockChainStore::getInstance()->processProducerEnding(signedBlockWrapperMessageProtoHelper)) {
        KETO_LOG_DEBUG << "[BlockProducer::processProducerEnding] This is the producer and needs to be triggered : "
                      << signedBlockWrapperMessageProtoHelper.getMessageHash().getHash(keto::common::StringEncoding::HEX);
        if (_getState() == BlockProducer::State::block_producer_wait) {
            KETO_LOG_INFO << "[BlockProducer::processProducerEnding] Trigger the producer : "
                          << signedBlockWrapperMessageProtoHelper.getMessageHash().getHash(keto::common::StringEncoding::HEX);
            _setState(BlockProducer::State::block_producer);
        }
    }
}

void BlockProducer::activateWaitingBlockProducer() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (_getState() == BlockProducer::State::block_producer_wait) {
        KETO_LOG_INFO << "[BlockProducer::activateWaitingBlockProducer] Activate waiting block producer" ;
        _setState(BlockProducer::State::block_producer);
    }
}

// get transaction lock
BlockProducer::ProducerScopeLockPtr BlockProducer::aquireTransactionLock() {
    return this->producerLockPtr->aquireTransactionLock();
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
        std::cv_status monitorStatus = this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(delay));
        if (this->currentState == BlockProducer::State::block_producer) {
            delay = 0;
            return this->currentState;
        }
        if (this->currentState == BlockProducer::State::block_producer_wait) {
            return this->currentState;
        }
        // removed the force resync this is now handled by the delay in shut down at the end of an election period.
        /*if (monitorStatus == std::cv_status::timeout && this->currentState == BlockProducer::State::block_producer_wait) {
            delay  = 0;
            BlockSyncManager::getInstance()->forceResync();
            return this->currentState;
        }*/
    } else {
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(Constants::BLOCK_TIME));
    }
    KETO_LOG_DEBUG << "[BlockProducer::checkState] return the state";
    if (this->currentState == BlockProducer::State::terminated) {
        return this->currentState;
    }
    return result;
}


void BlockProducer::processTransactions() {
    if (keto::software_consensus::ConsensusStateManager::getInstance()->getState() != keto::software_consensus::ConsensusStateManager::State::ACCEPTED) {
        return;
    }
    BlockProducer::ProducerScopeLockPtr producerScopeLockPtr = this->producerLockPtr->aquireBlockLock();

    // clear the account cache
    keto::proto::ClearDirtyCache clearDirtyCache;
    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::ClearDirtyCache>(
                    keto::server_common::Events::CLEAR_DIRTY_CACHE,clearDirtyCache));


    keto::block_db::SignedBlockWrapperMessageProtoHelper signedBlockWrapperMessageProtoHelper(
            keto::server_common::ServerInfo::getInstance()->getAccountHash());
    signedBlockWrapperMessageProtoHelper.setTangles(keto::block_db::BlockChainStore::getInstance()->getActiveTangles());
    signedBlockWrapperMessageProtoHelper.setProducerEnding(getProducerState()==BlockProducer::ProducerState::ending);
    // create a new transaction

    // loop through and attemp to create transactions
    do {
        keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
        try {
            std::deque<BlockProducer::PendingTransactionsTanglePtr> transactions = this->pendingTransactionManagerPtr->takeTransactions();
            for (BlockProducer::PendingTransactionsTanglePtr pendingTransactionTanglePtr : transactions) {
                keto::block_db::BlockChainStore::getInstance()->setCurrentTangle(
                        pendingTransactionTanglePtr->getTangle()->getTangleHash());
                keto::block_db::SignedBlockBuilderPtr signedBlockBuilderPtr = generateBlock(
                        pendingTransactionTanglePtr);
                if (signedBlockBuilderPtr) {
                    keto::block_db::SignedBlockWrapperProtoHelper signedBlockWrapperProtoHelper(signedBlockBuilderPtr);
                    signedBlockWrapperMessageProtoHelper.addSignedBlockWrapper(signedBlockWrapperProtoHelper);
                }
            }
            BlockSyncManager::getInstance()->broadcastBlock(signedBlockWrapperMessageProtoHelper);
            // producer state
            {
                std::unique_lock<std::mutex> uniqueLock(this->classMutex);
                if (signedBlockWrapperMessageProtoHelper.getProducerEnding()) {
                    KETO_LOG_INFO
                        << "[BlockProducer::processTransactions] Deactivate the block producer with the following block: "
                        << signedBlockWrapperMessageProtoHelper.getMessageHash().getHash(
                                keto::common::StringEncoding::HEX);
                    _setProducerState(BlockProducer::ProducerState::complete, true);
                    // clear the active tangles
                    keto::block_db::BlockChainStore::getInstance()->clearActiveTangles();
                    _setState(BlockProducer::State::sync_blocks);
                }
            }
            transactionPtr->commit();
            return;
        } catch (keto::common::Exception &ex) {
            KETO_LOG_ERROR << "[BlockProducer::processTransactions]Failed to create a new block: " << ex.what();
            KETO_LOG_ERROR << "[BlockProducer::processTransactions]Cause: " << boost::diagnostic_information(ex, true);
            transactionPtr->rollback();
            return;
        } catch (boost::exception &ex) {
            KETO_LOG_ERROR << "[BlockProducer::processTransactions]Failed to create a new block: "
                           << boost::diagnostic_information(ex, true);
            transactionPtr->rollback();
            return;
        } catch (std::exception &ex) {
            KETO_LOG_ERROR << "[BlockProducer::processTransactions]Failed to create a new block: " << ex.what();
            transactionPtr->rollback();
            return;
        } catch (...) {
            KETO_LOG_ERROR << "[BlockProducer::processTransactions]Failed to create a new block: unknown cause";
            transactionPtr->rollback();
            return;
        }
    } while (this->checkState() != State::terminated);
}


keto::block_db::SignedBlockBuilderPtr BlockProducer::generateBlock(const BlockProducer::PendingTransactionsTanglePtr& pendingTransactionTanglePtr) {

    try {

        keto::asn1::HashHelper parentHash = pendingTransactionTanglePtr->getTangle()->getLastBlockHash();
        if (parentHash.empty()) {
            KETO_LOG_INFO << "The block producer is not initialized";
            return keto::block_db::SignedBlockBuilderPtr();
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
        keto::block_db::BlockChainStore::getInstance()->writeBlock(signedBlockBuilderPtr,
                BlockChainCallbackImpl(getProducerState() == BlockProducer::ProducerState::ending));
        KETO_LOG_INFO << "Wrote a block [" <<
                      signedBlockBuilderPtr->getHash().getHash(keto::common::StringEncoding::HEX)
                      << "]";

        return signedBlockBuilderPtr;
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
    return keto::block_db::SignedBlockBuilderPtr();
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
        KETO_LOG_INFO << "[BlockProducer::load] loaded and check if genesis is required : " << this->getState();
        if (this->getState() == BlockProducer::State::block_producer) {
            if (BlockService::getInstance()->genesis()) {
                BlockSyncManager::getInstance()->notifyPeers(BlockSyncManager::COMPLETE);
            } else {
                KETO_LOG_INFO << "[BlockProducer::load] request network state from peers.";
                this->requestNetworkState();
            }
        } else {
            KETO_LOG_INFO << "[BlockProducer::load] force request of network information from peers.";
            this->requestNetworkState();
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
        while (this->getState() != BlockProducer::State::terminated &&
                this->getState() == BlockProducer::State::sync_blocks &&
            BlockSyncManager::getInstance()->getStatus() != BlockSyncManager::COMPLETE) {
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


void BlockProducer::waitingBlockProducerSync() {
    try {
        while (this->getState() != BlockProducer::State::terminated &&
               this->getState() == BlockProducer::State::block_producer_wait &&
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



    void BlockProducer::setProducerState(const  BlockProducer::ProducerState& state, bool notify) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    _setProducerState(state,notify);
}

void BlockProducer::_setProducerState(const BlockProducer::ProducerState& state, bool notify) {
    this->producerState = state;
    if (notify) {
        this->stateCondition.notify_all();
    }
}

BlockProducer::ProducerState BlockProducer::getProducerState() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->_getProducerState();
}


BlockProducer::ProducerState BlockProducer::_getProducerState() {
    return this->producerState;
}


void BlockProducer::requestNetworkState() {
    // assume we are out of date an force the sync
    _setState(BlockProducer::State::sync_blocks);
    keto::proto::RequestNetworkState requestNetworkState;
    // these method are currently not completely implemented but are there as a means to sync with the network state
    // should this later be required.
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RequestNetworkState>(
            keto::server_common::Events::REQUEST_NETWORK_STATE_SERVER,requestNetworkState));
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::RequestNetworkState>(
            keto::server_common::Events::REQUEST_NETWORK_STATE_CLIENT,requestNetworkState));
}


}
}