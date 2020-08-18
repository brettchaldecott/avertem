/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusServer.cpp
 * Author: ubuntu
 * 
 * Created on August 12, 2018, 8:38 AM
 */

#include <botan/hex.h>
#include <botan/base64.h>


#include "keto/crypto/HashGenerator.hpp"
#include "keto/environment/EnvironmentManager.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/TransactionHelper.hpp"


#include "keto/software_consensus/ConsensusSessionManager.hpp"

#include "keto/consensus_module/Constants.hpp"
#include "keto/consensus_module/ConsensusServer.hpp"

#include "keto/software_consensus/Constants.hpp"
#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "include/keto/consensus_module/ConsensusServer.hpp"


namespace keto {
namespace consensus_module {

ConsensusServer* consensusServerPtr = NULL;

void process(const boost::system::error_code& error) {
    consensusServerPtr->process();
}

const int ConsensusServer::THREAD_COUNT = 1;

ConsensusServer::ConsensusServer() :
    currentPos(-1),netwokSessionLength(Constants::NETWORK_SESSION_LENGTH_DEFAULT),
    netwokProtocolDelay(keto::software_consensus::Constants::NETWORK_PROTOCOL_DELAY_DEFAULT),
    networkHeartbeatDelay(Constants::NETWORK_CONSENSUS_HEARTBEAT_DEFAULT),
    networkHeartbeatCurrentSlot(0),
    networkHeartbeatElectionSlot(Constants::NETWORK_CONSENSUS_HEARTBEAT_ELECTION_SLOT_DEFAULT),
    networkHeartbeatElectionPublishSlot(Constants::NETWORK_CONSENSUS_HEARTBEAT_ELECTION_PUBLISH_SLOT_DEFAULT),
    networkHeartbeatConfirmationSlot(Constants::NETWORK_CONSENSUS_HEARTBEAT_CONFIRMATION_SLOT_DEFAULT)
    {
    networkHeartbeatPoint = networkPoint = sessionkeyPoint = std::chrono::system_clock::now();

    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = keto::environment::EnvironmentManager::getInstance()->getConfig();
    
    if (config->getVariablesMap().count(Constants::CONSENSUS_KEY)) {
        sessionKeys = keto::server_common::StringUtils(
                config->getVariablesMap()[Constants::CONSENSUS_KEY].as<std::string>()).tokenize(",");
    }
    if (config->getVariablesMap().count(Constants::NETWORK_SESSION_LENGTH_CONFIGURATION)) {
        this->netwokSessionLength =std::stol(
                config->getVariablesMap()[Constants::NETWORK_SESSION_LENGTH_CONFIGURATION].as<std::string>());
    }
    if (config->getVariablesMap().count(keto::software_consensus::Constants::NETWORK_PROTOCOL_DELAY_CONFIGURATION)) {
        this->netwokProtocolDelay =std::stol(
                config->getVariablesMap()[keto::software_consensus::Constants::NETWORK_PROTOCOL_DELAY_CONFIGURATION].as<std::string>());
    }
    if (config->getVariablesMap().count(Constants::NETWORK_CONSENSUS_HEARTBEAT_CONFIGURATION)) {
        this->networkHeartbeatDelay =std::stol(
                config->getVariablesMap()[Constants::NETWORK_CONSENSUS_HEARTBEAT_CONFIGURATION].as<std::string>());
    }
    if (config->getVariablesMap().count(Constants::NETWORK_CONSENSUS_HEARTBEAT_CONFIGURATION)) {
        this->networkHeartbeatDelay =std::stol(
                config->getVariablesMap()[Constants::NETWORK_CONSENSUS_HEARTBEAT_CONFIGURATION].as<std::string>());
    }
    if (config->getVariablesMap().count(Constants::NETWORK_CONSENSUS_HEARTBEAT_NETWORK_ELECTION_SLOT)) {
        this->networkHeartbeatElectionSlot =std::stol(
                config->getVariablesMap()[Constants::NETWORK_CONSENSUS_HEARTBEAT_NETWORK_ELECTION_SLOT].as<std::string>());
    }
    if (config->getVariablesMap().count(Constants::NETWORK_CONSENSUS_HEARTBEAT_NETWORK_ELECTION_PUBLISH_SLOT)) {
        this->networkHeartbeatElectionPublishSlot =std::stol(
                config->getVariablesMap()[Constants::NETWORK_CONSENSUS_HEARTBEAT_NETWORK_ELECTION_PUBLISH_SLOT].as<std::string>());
    }
    if (config->getVariablesMap().count(Constants::NETWORK_CONSENSUS_HEARTBEAT_NETWORK_CONFIRMATION_SLOT)) {
        this->networkHeartbeatConfirmationSlot =std::stol(
                config->getVariablesMap()[Constants::NETWORK_CONSENSUS_HEARTBEAT_NETWORK_CONFIRMATION_SLOT].as<std::string>());
    }


    consensusServerPtr = this;
}

ConsensusServer::~ConsensusServer() {
    consensusServerPtr = NULL;
    if (this->ioc) {
        this->ioc->stop();

        for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
                iter != this->threadsVector.end(); iter++) {
            iter->join();
        }

        this->threadsVector.clear();
    }
}

bool ConsensusServer::require() {
    if (sessionKeys.size()) {
        return true;
    }
    return false;
}

void ConsensusServer::start() {
    // The io_context is required for all I/O
    this->ioc = std::make_shared<boost::asio::io_context>(THREAD_COUNT);
    
    boost::posix_time::seconds interval(0);  // 1 second
    this->timer = std::make_shared<boost::asio::deadline_timer>(*this->ioc.get());
    this->timer->expires_from_now(interval);
    this->timer->async_wait(&keto::consensus_module::process);
    
    this->threadsVector.reserve(THREAD_COUNT);
    for(int i = 0; i < THREAD_COUNT; i++) {
        this->threadsVector.emplace_back(
        [this]
        {
            this->ioc->run();
        });
    }
}


void ConsensusServer::process() {
    try {
        std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
        std::chrono::minutes diff(
                std::chrono::duration_cast<std::chrono::minutes>(currentTime - this->sessionkeyPoint));
        // check if the network time is up and needs to be retested
        std::chrono::minutes networkDiff(
                std::chrono::duration_cast<std::chrono::minutes>(currentTime - this->networkPoint));
        std::chrono::seconds heartbeatDiff(
                std::chrono::duration_cast<std::chrono::seconds>(currentTime - this->networkHeartbeatPoint));
        if ((this->currentPos == -1) || (diff.count() > this->netwokSessionLength)) {
            KETO_LOG_INFO << "[ConsensusServer::process] Release a new session key";
            this->currentPos++;
            if (this->currentPos >= this->sessionKeys.size()) {
                this->currentPos = 0;
            }
            keto::crypto::SecureVector initVector = Botan::hex_decode_locked(
                    this->sessionKeys[this->currentPos], true);
            keto::software_consensus::ConsensusSessionManager::getInstance()->updateSessionKey(initVector);
            internalConsensusInit(keto::crypto::HashGenerator().generateHash(initVector));
            this->networkHeartbeatPoint = this->networkPoint = this->sessionkeyPoint = std::chrono::system_clock::now();
            this->networkHeartbeatCurrentSlot = 0;
        } else if (networkDiff.count() > this->netwokProtocolDelay) {
            KETO_LOG_INFO << "[ConsensusServer::process] Time to retest the network.";
            keto::crypto::SecureVector initVector = Botan::hex_decode_locked(
                    this->sessionKeys[this->currentPos], true);
            long time = currentTime.time_since_epoch().count();
            const uint8_t* ptr =  (uint8_t*)&time;
            keto::crypto::SecureVector timeBytesVector(ptr,ptr+4);
            initVector.insert(initVector.begin(),timeBytesVector.begin(),timeBytesVector.end());
            internalConsensusProtocolCheck(keto::crypto::HashGenerator().generateHash(initVector));
            std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
            this->networkHeartbeatPoint = this->networkPoint = endTime;
            this->networkHeartbeatCurrentSlot = 0;
            // recalculate the points
            this->sessionkeyPoint = this->sessionkeyPoint + (endTime - currentTime);

        } else if (heartbeatDiff.count() > this->networkHeartbeatDelay) {
            KETO_LOG_INFO << "[ConsensusServer::process] The network heartbeat.";
            initNetworkHeartbeat();
            std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
            this->networkHeartbeatPoint = endTime;

            // reculculate the points
            this->networkPoint = this->networkPoint + (endTime - currentTime);
            this->sessionkeyPoint = this->sessionkeyPoint + (endTime - currentTime);
        }

    } catch (keto::common::Exception &ex) {
        KETO_LOG_ERROR << "[ConsensusServer] Failed setup the consensus : "
                       << boost::diagnostic_information(ex, true) << std::endl
                       << boost::diagnostic_information_what(ex, true);
    } catch (boost::exception &ex) {
        KETO_LOG_ERROR << "[ConsensusServer] Failed setup the consensus : "
                       << boost::diagnostic_information_what(ex, true);
    } catch (std::exception &ex) {
        KETO_LOG_ERROR << "[ConsensusServer] Failed setup the consensus : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[ConsensusServer] Failed setup the consensus : unknown error";
    }
    this->reschedule();
}


void ConsensusServer::internalConsensusInit(const keto::crypto::SecureVector& initHash) {
    //KETO_LOG_DEBUG << "Setup the internal consensus";
    keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper;
    moduleHashMessageHelper.setHash(initHash);
    keto::proto::ModuleHashMessage moduleHashMessage = moduleHashMessageHelper.getModuleHashMessage();
    keto::proto::ConsensusMessage consensusMessage =
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ModuleHashMessage>(
                                    keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,moduleHashMessage)));
    keto::software_consensus::ConsensusSessionManager::getInstance()->setSession(consensusMessage);

    keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyAccepted();
    transactionPtr->commit();



}


void ConsensusServer::internalConsensusProtocolCheck(const keto::crypto::SecureVector& initHash) {
    // reset the protocol check
    keto::software_consensus::ConsensusSessionManager::getInstance()->resetProtocolCheck();

    //KETO_LOG_DEBUG << "Setup the internal consensus";
    keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper;
    moduleHashMessageHelper.setHash(initHash);
    keto::proto::ModuleHashMessage moduleHashMessage = moduleHashMessageHelper.getModuleHashMessage();
    keto::proto::ConsensusMessage consensusMessage =
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ModuleHashMessage>(
                                    keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,moduleHashMessage)));
    keto::software_consensus::ConsensusSessionManager::getInstance()->setSession(consensusMessage);

    keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
    keto::software_consensus::ConsensusSessionManager::getInstance()->notifyProtocolCheck(true);
    transactionPtr->commit();

}

void ConsensusServer::initNetworkHeartbeat() {
    keto::transaction::TransactionPtr transactionPtr = keto::server_common::createTransaction();
    keto::software_consensus::ConsensusSessionManager::getInstance()->initNetworkHeartbeat(
            this->networkHeartbeatCurrentSlot++, this->networkHeartbeatElectionSlot, this->networkHeartbeatElectionPublishSlot, this->networkHeartbeatConfirmationSlot);
    transactionPtr->commit();
}

void ConsensusServer::reschedule() {
    this->timer->expires_from_now(boost::posix_time::seconds(10));
    this->timer->async_wait(&keto::consensus_module::process);
}

}
}
