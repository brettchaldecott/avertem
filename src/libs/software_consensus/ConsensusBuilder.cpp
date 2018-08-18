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

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/ServerInfo.hpp"

#include "keto/software_consensus/Constants.hpp"
#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/software_consensus/ConsensusBuilder.hpp"
#include "keto/software_consensus/ModuleConsensusHelper.hpp"

#include "keto/server_common/ServerInfo.hpp"
#include "include/keto/software_consensus/ModuleConsensusHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ConsensusBuilder::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

ConsensusBuilder::ConsensusBuilder(
        const ConsensusHashGeneratorPtr consensusHashGeneratorPtr,
        const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) : 
    consensusHashGeneratorPtr(consensusHashGeneratorPtr),
    keyLoaderPtr(keyLoaderPtr)
{
    std::vector<uint8_t> accountHash = 
            keto::server_common::ServerInfo::getInstance()->getAccountHash();
    consensusMessageHelper.setAccountHash(accountHash);
}

ConsensusBuilder::ConsensusBuilder(
        const keto::proto::ConsensusMessage& consensusMessage) : 
    consensusMessageHelper(consensusMessage)
{
}


ConsensusBuilder::~ConsensusBuilder() {
    
}

ConsensusBuilder& ConsensusBuilder::buildConsensus(const keto::asn1::HashHelper& previousHash) {
    keto::software_consensus::SoftwareConsensusHelper softwareConsensusHelper;
    
    softwareConsensusHelper.setPreviousHash(previousHash);
    
    keto::asn1::HashHelper seedHash(this->consensusHashGeneratorPtr->generateSeed(previousHash));
    
    //SoftwareConsensusHelper
    softwareConsensusHelper.setSeed(seedHash).
        setAccount(keto::asn1::HashHelper(keto::crypto::SecureVectorUtils().copyToSecure(
        keto::server_common::ServerInfo::getInstance()->getAccountHash()))).
        setDate(keto::asn1::TimeHelper());
    for (std::string event : Constants::EVENT_ORDER) {
        try {
            keto::software_consensus::ModuleConsensusHelper moduleConsensusHelper;
            moduleConsensusHelper.setSeedHash(seedHash);
            keto::proto::ModuleConsensusMessage moduleeConsensusMessage = 
                    moduleConsensusHelper.getModuleConsensusMessage();
            keto::software_consensus::ModuleConsensusHelper moduleConsensusResult(
                keto::server_common::fromEvent<keto::proto::ModuleConsensusMessage>(
                    keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::ModuleConsensusMessage>(
                    event,moduleeConsensusMessage))));
            softwareConsensusHelper.addSystemHash(moduleConsensusResult.getModuleHash());
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
    softwareConsensusHelper.generateMerkelRoot();
    softwareConsensusHelper.sign(this->keyLoaderPtr);
    consensusMessageHelper.setMsg(softwareConsensusHelper);
    return *this;
}


bool ConsensusBuilder::validateConsensus() {
    keto::software_consensus::SoftwareConsensusHelper softwareConsensusHelper =
        consensusMessageHelper.getMsg();
    std::vector<keto::asn1::HashHelper> systemHashes = 
            softwareConsensusHelper.getSystemHashes();
    std::vector<keto::asn1::HashHelper> comparedHashes;
    for (std::string event : Constants::EVENT_ORDER) {
        try {
            keto::software_consensus::ModuleConsensusHelper moduleConsensusHelper;
            moduleConsensusHelper.setSeedHash(softwareConsensusHelper.getSeed());
            keto::proto::ModuleConsensusMessage moduleConsensusMessage = 
                    moduleConsensusHelper.getModuleConsensusMessage();
            keto::software_consensus::ModuleConsensusHelper moduleConsensusResult(
                keto::server_common::fromEvent<keto::proto::ModuleConsensusMessage>(
                    keto::server_common::processEvent(
                    keto::server_common::toEvent<keto::proto::ModuleConsensusMessage>(
                    event,moduleConsensusMessage))));
            for (std::vector<keto::asn1::HashHelper>::iterator iter = 
                    systemHashes.begin(); iter != systemHashes.end(); iter++) {
                if (iter->operator keto::crypto::SecureVector() ==
                        moduleConsensusResult.getModuleHash().operator keto::crypto::SecureVector()) {
                    systemHashes.erase(iter);
                    comparedHashes.push_back(moduleConsensusResult.getModuleHash());
                    break;
                }
            }
            
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
    
    if (systemHashes.size()) {
        return false;
    }
    return true;
}

ConsensusMessageHelper ConsensusBuilder::getConsensus() {
    return this->consensusMessageHelper;
}


}
}