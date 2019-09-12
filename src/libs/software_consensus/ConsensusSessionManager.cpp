/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusSessionManager.cpp
 * Author: ubuntu
 * 
 * Created on July 18, 2018, 5:25 PM
 */

#include "SoftwareConsensus.pb.h"

#include "botan/hex.h"

#include "keto/common/Log.hpp"

#include "keto/crypto/HashGenerator.hpp"

#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/event/Event.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/EventUtils.hpp"

#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleSessionMessageHelper.hpp"
#include "keto/software_consensus/Constants.hpp"
#include "keto/software_consensus/ConsensusAcceptedMessageHelper.hpp"
#include "keto/software_consensus/ProtocolAcceptedMessageHelper.hpp"
#include "keto/software_consensus/Exception.hpp"

#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"


namespace keto {
namespace software_consensus {

static ConsensusSessionManagerPtr singleton;

std::string ConsensusSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
ConsensusSessionManager::ConsensusSessionManager() : activeSession(false), accepted(false),
    netwokProtocolDelay(Constants::NETWORK_PROTOCOL_DELAY_DEFAULT), networkProtocolCount(Constants::NETWORK_PROTOCOL_COUNT_DEFAULT),
    protolCount(-1){
    this->protocolPoint = std::chrono::system_clock::now();

    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = keto::environment::EnvironmentManager::getInstance()->getConfig();

    if (config->getVariablesMap().count(Constants::NETWORK_PROTOCOL_DELAY_CONFIGURATION)) {
        this->netwokProtocolDelay =std::stol(
                config->getVariablesMap()[Constants::NETWORK_PROTOCOL_DELAY_CONFIGURATION].as<std::string>());
    }
    if (config->getVariablesMap().count(Constants::NETWORK_PROTOCOL_DELAY_CONFIGURATION)) {
        this->networkProtocolCount =std::stol(
                config->getVariablesMap()[Constants::NETWORK_PROTOCOL_COUNT_CONFIGURATION].as<std::string>());
    }
}

ConsensusSessionManager::~ConsensusSessionManager() {
}

ConsensusSessionManagerPtr ConsensusSessionManager::init() {
    return singleton = ConsensusSessionManagerPtr(new ConsensusSessionManager());
}

ConsensusSessionManagerPtr ConsensusSessionManager::getInstance() {
    return singleton;
}

void ConsensusSessionManager::fin() {
    singleton.reset();
}


void ConsensusSessionManager::updateSessionKey(const keto::crypto::SecureVector& sessionKey) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    // the session key is zero length
    if (!sessionKey.size()) {
        KETO_LOG_ERROR << "[updateSessionKey] The session key is zero lenght ignore update";
        BOOST_THROW_EXCEPTION(keto::software_consensus::InvalidSessionException());
    }
    keto::crypto::SecureVector sessionHash = keto::crypto::HashGenerator().generateHash(sessionKey);

    if (sessionHash == this->sessionHash) {
        // ignore at this point the session matches and we dont need to update it.
        KETO_LOG_DEBUG << "[updateSessionKey] Ignore the session key : " << Botan::hex_encode((uint8_t*)sessionHash.data(),sessionHash.size(),true);
        return;
    } else {
        KETO_LOG_DEBUG << "[updateSessionKey] Start a new session as the session hashes dont match : " << Botan::hex_encode((uint8_t*)sessionHash.data(),sessionHash.size(),true);
    }
    this->sessionHash = sessionHash;
    this->accepted = false;
    this->activeSession = false;
    for (std::string event : Constants::CONSENSUS_SESSION_ORDER) {
        try {
            keto::software_consensus::ModuleSessionMessageHelper moduleSessionMessageHelper;
            moduleSessionMessageHelper.setSecret(sessionKey);
            keto::proto::ModuleSessionMessage moduleSessionMessage = 
                    moduleSessionMessageHelper.getModuleSessionMessage();
            keto::server_common::triggerEvent(
                    keto::server_common::toEvent<keto::proto::ModuleSessionMessage>(
                    event,moduleSessionMessage));
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event  << "] : " << ex.what();
            KETO_LOG_ERROR << "[updateSessionKey]Cause: " << boost::diagnostic_information(ex,true);
            BOOST_THROW_EXCEPTION(keto::software_consensus::InvalidSessionException());
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "[updateSessionKey]Cause: " << boost::diagnostic_information(ex,true);
            BOOST_THROW_EXCEPTION(keto::software_consensus::InvalidSessionException());
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "[updateSessionKey]The cause is : " << ex.what();
            BOOST_THROW_EXCEPTION(keto::software_consensus::InvalidSessionException());
        } catch (...) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event << "]";
            BOOST_THROW_EXCEPTION(keto::software_consensus::InvalidSessionException());
        }
    }
}


void ConsensusSessionManager::setSession(keto::proto::ConsensusMessage& msg) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (!this->activeSession) {
        this->activeSession = true;
        // setup the consensus message
        for (std::string event : Constants::CONSENSUS_SESSION_STATE) {
            try {
                keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                        keto::server_common::processEvent(
                                keto::server_common::toEvent<keto::proto::ConsensusMessage>(
                                        event, msg)));
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[setSession]Failed to process the event [" << event  << "] : " << ex.what();
                KETO_LOG_ERROR << "[setSession]Cause: " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[setSession]Failed to process the event [" << event << "]";
                KETO_LOG_ERROR << "[setSession]Cause: " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[setSession]Failed to process the event [" << event << "]";
                KETO_LOG_ERROR << "[setSession]The cause is : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[setSession]Failed to process the event [" << event << "]";
            }
        }

    }
}

void ConsensusSessionManager::notifyAccepted() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (!this->accepted) {
        this->accepted = true;
        for (std::string event : Constants::CONSENSUS_SESSION_ACCEPTED) {
            try {
                keto::software_consensus::ConsensusAcceptedMessageHelper consensusAcceptedMessageHelper;
                keto::proto::ConsensusAcceptedMessage msg =
                        consensusAcceptedMessageHelper.getMsg();
                keto::server_common::triggerEvent(
                        keto::server_common::toEvent<keto::proto::ConsensusAcceptedMessage>(
                                event,msg));
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[notifyAccepted]Failed to process the event [" << event  << "] : " << ex.what();
                KETO_LOG_ERROR << "[notifyAccepted]Cause: " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[notifyAccepted]Failed to process the event [" << event << "]";
                KETO_LOG_ERROR << "[notifyAccepted]Cause: " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[notifyAccepted]Failed to process the event [" << event << "]";
                KETO_LOG_ERROR << "[notifyAccepted]The cause is : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[notifyAccepted]Failed to process the event [" << event << "]";
            }
        }
    }
}

bool ConsensusSessionManager::resetProtocolCheck() {
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::minutes networkDiff(
            std::chrono::duration_cast<std::chrono::minutes>(currentTime - this->protocolPoint));
    if (this->protolCount == -1 || networkDiff.count() >= this->netwokProtocolDelay) {
        this->protocolPoint = currentTime;
        this->protolCount = 0;
        return true;
    }
    return false;
}

void ConsensusSessionManager::notifyProtocolCheck(bool master) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (master || this->protolCount == this->networkProtocolCount) {
        for (std::string event : Constants::CONSENSUS_SESSION_CHECK) {
            try {
                keto::software_consensus::ProtocolAcceptedMessageHelper protocolAcceptedMessageHelper;
                keto::proto::ProtocolAcceptedMessage msg =
                        protocolAcceptedMessageHelper.getMsg();
                keto::server_common::triggerEvent(
                        keto::server_common::toEvent<keto::proto::ProtocolAcceptedMessage>(
                                event,msg));
            } catch (keto::common::Exception& ex) {
                KETO_LOG_ERROR << "[notifyProtocolCheck]Failed to process the event [" << event  << "] : " << ex.what();
                KETO_LOG_ERROR << "[notifyProtocolCheck]Cause: " << boost::diagnostic_information(ex,true);
            } catch (boost::exception& ex) {
                KETO_LOG_ERROR << "[notifyProtocolCheck]Failed to process the event [" << event << "]";
                KETO_LOG_ERROR << "[notifyProtocolCheck]Cause: " << boost::diagnostic_information(ex,true);
            } catch (std::exception& ex) {
                KETO_LOG_ERROR << "[notifyProtocolCheck]Failed to process the event [" << event << "]";
                KETO_LOG_ERROR << "[notifyProtocolCheck]The cause is : " << ex.what();
            } catch (...) {
                KETO_LOG_ERROR << "[notifyProtocolCheck]Failed to process the event [" << event << "]";
            }
        }
    }
    this->protolCount++;
}


void ConsensusSessionManager::initNetworkHeartbeat(int networkSlot, int electionSlot, int electionPublishSlot, int confirmationSlot) {
    keto::software_consensus::ProtocolHeartbeatMessageHelper protocolHeartbeatMessageHelper;
    protocolHeartbeatMessageHelper.setNetworkSlot(networkSlot);
    protocolHeartbeatMessageHelper.setElectionSlot(electionSlot);
    protocolHeartbeatMessageHelper.setElectionPublishSlot(electionPublishSlot);
    protocolHeartbeatMessageHelper.setConfirmationSlot(confirmationSlot);
    initNetworkHeartbeat(protocolHeartbeatMessageHelper.getMsg());
}

void ConsensusSessionManager::initNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& msg) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (!checkHeartbeatTimestamp(msg)) {
        return;
    }
    for (std::string event : Constants::CONSENSUS_HEARTBEAT) {
        try {
            keto::server_common::triggerEvent(
                    keto::server_common::toEvent<keto::proto::ProtocolHeartbeatMessage>(
                            event,msg));
        } catch (keto::common::Exception& ex) {
            KETO_LOG_ERROR << "[initNetworkHeartbeat]Failed to process the event [" << event  << "] : " << ex.what();
            KETO_LOG_ERROR << "[initNetworkHeartbeat]Cause: " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[initNetworkHeartbeat]Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "[initNetworkHeartbeat]Cause: " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[initNetworkHeartbeat]Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "[initNetworkHeartbeat]The cause is : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[initNetworkHeartbeat]Failed to process the event [" << event << "]";
        }
    }
}


bool ConsensusSessionManager::checkHeartbeatTimestamp(const keto::proto::ProtocolHeartbeatMessage& msg) {
    if (this->protocolHeartbeatMessageHelper.getTimestamp() < msg.timestamp()) {
        this->protocolHeartbeatMessageHelper = ProtocolHeartbeatMessageHelper(msg);
        return true;
    }
    return false;
}




}
}