/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerInfo.cpp
 * Author: ubuntu
 * 
 * Created on February 16, 2018, 8:31 AM
 */

#include <mutex>
#include <sstream>

#include <botan/hex.h>
#include <keto/server_common/Events.hpp>

#include "KeyStore.pb.h"

#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/Constants.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/server_common/Exception.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"


namespace keto {
namespace server_common {

static std::mutex singletonMutex;
static std::shared_ptr<ServerInfo> singleton;

std::string ServerInfo::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


ServerInfo::ServerInfo() : checkedMaster(false), master(false){
    
    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    
    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::server_common::Constants::PUBLIC_KEY_DIR)) {
        BOOST_THROW_EXCEPTION(keto::server_common::NoPublicKeyDirectoryConfiguredException());
    }
    std::string publicKeyPath = 
            config->getVariablesMap()[keto::server_common::Constants::PUBLIC_KEY_DIR].as<std::string>();
    this->publicKeyPath =  
            keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / publicKeyPath;
    if (!boost::filesystem::exists(this->publicKeyPath)) {
        std::stringstream ss;
        ss << "The public key path is invalid : " << this->publicKeyPath.string();
        BOOST_THROW_EXCEPTION(keto::server_common::InvalidPublicKeyDirectoryException(
                ss.str()));
    }
    
    if (!config->getVariablesMap().count(keto::server_common::Constants::ACCOUNT_HASH)) {
        BOOST_THROW_EXCEPTION(keto::server_common::NoServerAccountConfiguredException());
    }
    std::string accountHash = 
            config->getVariablesMap()[keto::server_common::Constants::ACCOUNT_HASH].as<std::string>();
    
    this->accountHash = Botan::hex_decode(accountHash,true);
    
    this->feeAccountHash = this->accountHash;
    if (config->getVariablesMap().count(keto::server_common::Constants::FEE_ACCOUNT_HASH)) {
        accountHash = 
            config->getVariablesMap()[keto::server_common::Constants::FEE_ACCOUNT_HASH].as<std::string>();
    
        this->feeAccountHash = Botan::hex_decode(accountHash,true);
    } 
}

ServerInfo::~ServerInfo() {
}

std::shared_ptr<ServerInfo> ServerInfo::getInstance() {
    std::lock_guard<std::mutex> guard(singletonMutex);
    if (!singleton) {
        singleton = std::shared_ptr<ServerInfo>(new ServerInfo());
    }
    return singleton;
}

std::vector<uint8_t> ServerInfo::getAccountHash() {
    return this->accountHash;
}

std::vector<uint8_t> ServerInfo::getFeeAccountHash() {
    return this->feeAccountHash;
}

std::string ServerInfo::getFeeAccountHashHex() {
    return Botan::hex_encode(this->feeAccountHash);
}

boost::filesystem::path ServerInfo::getPublicKeyPath() {
    return this->publicKeyPath;
}


bool ServerInfo::isMaster() {
    try {
        if (!checkedMaster) {
            keto::proto::MasterInfo masterInfo;

            masterInfo =
                    keto::server_common::fromEvent<keto::proto::MasterInfo>(
                            keto::server_common::processEvent(
                                    keto::server_common::toEvent<keto::proto::MasterInfo>(
                                            keto::server_common::Events::IS_MASTER, masterInfo)));
            checkedMaster = true;
            this->master = masterInfo.is_master();
        }
        return this->master;
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[isMaster]Failed check if this is a master: " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[isMaster]Failed check if this is a master : " << boost::diagnostic_information(ex,true);
        throw;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[isMaster]Failed check if this is a master : " << ex.what();
        throw;
    } catch (...) {
        KETO_LOG_ERROR << "Failed to remove the entry.";
        throw;
    }
}

}
}
