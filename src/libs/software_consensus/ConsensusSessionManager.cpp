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


namespace keto {
namespace software_consensus {

static ConsensusSessionManagerPtr singleton;

std::string ConsensusSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
ConsensusSessionManager::ConsensusSessionManager() : activeSession(false), accepted(false)  {
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
    keto::crypto::SecureVector sessionHash = keto::crypto::HashGenerator().generateHash(sessionKey);

    if (sessionHash == this->sessionHash) {
        // ignore at this point the session matches and we dont need to update it.
        return;
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
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "[updateSessionKey]Cause: " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "[updateSessionKey]The cause is : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "[updateSessionKey]Failed to process the event [" << event << "]";
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


}
}