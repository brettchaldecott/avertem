/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcSessionManager.cpp
 * Author: ubuntu
 * 
 * Created on January 22, 2018, 1:18 PM
 */

#include <map>

#include "keto/rpc_client/RpcSessionManager.hpp"
#include "include/keto/rpc_client/RpcSessionManager.hpp"
#include "include/keto/rpc_client/RpcSession.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/ssl/RootCertificate.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

namespace keto {
namespace rpc_client {

namespace ketoEnv = keto::environment;

std::string RpcSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

RpcSessionManager::RpcSessionManager()  {
    
    this->ioc = std::make_shared<boost::asio::io_context>();
    
    this->ctx = std::make_shared<boostSsl::context>(boostSsl::context::sslv23_client);

    // This holds the root certificate used for verification
    keto::ssl::load_root_certificates(*ctx);
    
    // retrieve the configuration
    std::shared_ptr<ketoEnv::Config> config = ketoEnv::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::PEERS)) {
        std::cout << "Load the peers" << std::endl;
        keto::server_common::StringVector peers = 
                keto::server_common::StringUtils(
                config->getVariablesMap()[Constants::PEERS].
                as<std::string>()).tokenize(",");
        std::cout << "Load the peers" << std::endl;
        for (std::vector<std::string>::iterator iter = peers.begin();
                iter != peers.end(); iter++) {
            std::cout << "The peer is : " << (*iter) << std::endl;
            this->sessionMap[(*iter)] = std::make_shared<RpcSession>(
                    this->ioc,
                    this->ctx,(*iter));
        }
    }
    
    threads = Constants::DEFAULT_RPC_CLIENT_THREADS;
    if (config->getVariablesMap().count(Constants::RPC_CLIENT_THREADS)) {
        threads = std::max<int>(1,atoi(config->getVariablesMap()[Constants::RPC_CLIENT_THREADS].as<std::string>().c_str()));
    }
}

RpcSessionManager::~RpcSessionManager() {
}


std::vector<std::string> RpcSessionManager::listPeers() {
    std::vector<std::string> keys;
    std::transform(
        this->sessionMap.begin(),
        this->sessionMap.end(),
        std::back_inserter(keys),
        [](const std::map<std::string,std::shared_ptr<keto::rpc_client::RpcSession>>::value_type 
            &pair){return pair.first;});
    return keys;
}



void RpcSessionManager::start() {
    std::cout << "The start has been called" << std::endl;
    // Run the I/O service on the requested number of threads
    
}

void RpcSessionManager::postStart() {
    std::cout << "The post start has been called" << std::endl;
    for (std::map<std::string,std::shared_ptr<RpcSession>>::iterator it=this->sessionMap.begin(); 
            it!=this->sessionMap.end(); ++it) {
        it->second->run();
    }
    this->threadsVector.reserve(this->threads);
    for(int i = 0; i < this->threads; i++) {
        this->threadsVector.emplace_back(
        [this]
        {
            this->ioc->run();
        });
    }
    std::cout << "All the threads have been started" << std::endl;
    
}

void RpcSessionManager::stop() {
    this->ioc->stop();
    
    for (std::vector<std::thread>::iterator iter = this->threadsVector.begin();
            iter != this->threadsVector.end(); iter++) {
        iter->join();
    }

    this->threadsVector.clear();
    
    this->sessionMap.clear();
}

}
}