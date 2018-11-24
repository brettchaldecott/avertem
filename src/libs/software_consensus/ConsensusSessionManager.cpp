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

#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/software_consensus/ConsensusSessionManager.hpp"
#include "keto/software_consensus/ModuleSessionMessageHelper.hpp"
#include "keto/software_consensus/Constants.hpp"


namespace keto {
namespace software_consensus {

std::string ConsensusSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
ConsensusSessionManager::ConsensusSessionManager() {
}

ConsensusSessionManager::~ConsensusSessionManager() {
}

void ConsensusSessionManager::updateSessionKey(const keto::crypto::SecureVector& sessionKey) {
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
            KETO_LOG_ERROR << "Failed to process the event [" << event  << "] : " << ex.what();
            KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
        } catch (boost::exception& ex) {
            KETO_LOG_ERROR << "Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
        } catch (std::exception& ex) {
            KETO_LOG_ERROR << "Failed to process the event [" << event << "]";
            KETO_LOG_ERROR << "The cause is : " << ex.what();
        } catch (...) {
            KETO_LOG_ERROR << "Failed to process the event [" << event << "]";
        }
    }
}

}
}