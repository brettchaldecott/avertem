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


#include "keto/environment/EnvironmentManager.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/software_consensus/ConsensusSessionManager.hpp"

#include "keto/consensus_module/Constants.hpp"
#include "keto/consensus_module/ConsensusServer.hpp"
#include "include/keto/consensus_module/ConsensusServer.hpp"


namespace keto {
namespace consensus_module {

ConsensusServer* consensusServerPtr = NULL;

void process(const boost::system::error_code& error) {
    consensusServerPtr->process();
}

const int ConsensusServer::THREAD_COUNT = 1;

ConsensusServer::ConsensusServer() : currentPos(-1) {
    time_point = std::chrono::system_clock::now();
    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = keto::environment::EnvironmentManager::getInstance()->getConfig();
    
    if (config->getVariablesMap().count(Constants::CONSENSUS_KEY)) {
        sessionKeys = keto::server_common::StringUtils(
                config->getVariablesMap()[Constants::CONSENSUS_KEY].as<std::string>()).tokenize(",");
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
    std::cout << "The process is saying high" << std::endl;
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::hours diff(
             std::chrono::duration_cast<std::chrono::hours>(currentTime-this->time_point));
    if ((this->currentPos == -1) || (diff.count() > 2)) {
        this->currentPos++;
        if (this->currentPos >= this->sessionKeys.size()) {
            this->currentPos = 0;
        }
        std::cout << "The session keys : " << this->sessionKeys[this->currentPos] << std::endl;
        keto::crypto::SecureVector initVector = Botan::hex_decode_locked(
                this->sessionKeys[this->currentPos],true);
        keto::software_consensus::ConsensusSessionManager().updateSessionKey(initVector);
        
        this->time_point = currentTime;
    }
    
    this->timer->expires_from_now(boost::posix_time::seconds(10));
    this->timer->async_wait(&keto::consensus_module::process);
}

}
}