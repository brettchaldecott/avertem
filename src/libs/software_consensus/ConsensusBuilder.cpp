/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusBuilder.cpp
 * Author: ubuntu
 * 
 * Created on May 28, 2018, 9:44 AM
 */

#include "SoftwareConsensus.pb.h"

#include "keto/common/Log.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"


#include "keto/software_consensus/Constants.hpp"
#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/server_common/ServerInfo.hpp"

namespace keto {
namespace software_consensus {


ConsensusBuilder::ConsensusBuilder(
        const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) : 
    consensusMessageHelper(keyLoaderPtr) {
    std::vector<uint8_t> accountHash = 
            keto::server_common::ServerInfo::getInstance()->getAccountHash();
    consensusMessageHelper.setAccountHash(accountHash);
}

ConsensusBuilder::~ConsensusBuilder() {
    
}

ConsensusBuilder& ConsensusBuilder::buildConsensus() {
    for (std::string event : Constants::EVENT_ORDER) {
        try {
            keto::proto::SoftwareConsensusMessage consensusMessage;
            
            consensusMessage = 
                keto::server_common::fromEvent<keto::proto::SoftwareConsensusMessage>(
                    keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::SoftwareConsensusMessage>(
                    event,consensusMessage)));
            keto::asn1::HashHelper hashHelper;
            hashHelper = consensusMessage.encrypted_data_response();
            consensusMessageHelper.addSystemHash(hashHelper);
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
    consensusMessageHelper.generateMerkelRoot();
    consensusMessageHelper.sign();
    return *this;
}

ConsensusMessageHelper ConsensusBuilder::getConsensus() {
    return this->consensusMessageHelper;
}


}
}