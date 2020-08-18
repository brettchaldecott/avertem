/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountGraphStore.cpp
 * Author: ubuntu
 * 
 * Created on March 5, 2018, 7:30 AM
 */

#include <sstream>
#include <iostream>

#include "KeyStore.pb.h"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


#include <boost/filesystem/path.hpp>
#include <keto/server_common/Events.hpp>

#include "keto/account_db/AccountGraphStore.hpp"
#include "keto/account_db/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"
#include "keto/account_db/Constants.hpp"



namespace keto {
namespace account_db {
    
std::string AccountGraphStore::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountGraphStore::StorageLock::StorageLock() : readLock(0), writeLock(0) {

}

AccountGraphStore::StorageLock::~StorageLock() {

}

AccountGraphStore::StorageScopeLockPtr AccountGraphStore::StorageLock::aquireReadLock() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    while(this->writeLock) {
        this->stateCondition.wait(uniqueLock);
    }
    this->readLock++;
    //KETO_LOG_ERROR << "[AccountGraphStore::StorageLock::aquireWriteLock]Aquire read lock [" << this->readLock << "][" <<this->writeLock << "]";
    return AccountGraphStore::StorageScopeLockPtr(new AccountGraphStore::StorageScopeLock(this,true,false));
}

AccountGraphStore::StorageScopeLockPtr AccountGraphStore::StorageLock::aquireWriteLock() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    // write lock must be unique
    while(this->writeLock) {
        this->stateCondition.wait(uniqueLock);
    }
    this->writeLock++;
    while(this->readLock){
        this->stateCondition.wait(uniqueLock);
    }
    //KETO_LOG_ERROR << "[AccountGraphStore::StorageLock::aquireWriteLock]Aquire write lock [" << this->readLock << "][" <<this->writeLock << "]";
    return AccountGraphStore::StorageScopeLockPtr(new AccountGraphStore::StorageScopeLock(this,false,true));
}

void AccountGraphStore::StorageLock::release(bool _readLock, bool _writeLock) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    //KETO_LOG_ERROR << "[AccountGraphStore::StorageLock::release]Release the lock [" << this->readLock << "][" <<this->writeLock << "]";
    if (_readLock) {
        this->readLock--;
    } else if (_writeLock) {
        this->writeLock--;
    }
    // use notify all this is cumbersome but a lot more effective than having aquires individually release until a lock is
    // aquired
    this->stateCondition.notify_all();
}

AccountGraphStore::StorageScopeLock::StorageScopeLock(StorageLock* reference, bool readLock, bool writeLock)
    : reference(reference), readLock(readLock), writeLock(writeLock) {
}

AccountGraphStore::StorageScopeLock::~StorageScopeLock() {
    this->reference->release(this->readLock,this->writeLock);
}

AccountGraphStore::AccountGraphStore(const std::string& dbName) : dbName(dbName) {
    // setup the world
    world = librdf_new_world();
    librdf_world_open(world);

    // setup the bdb hash db
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::GRAPH_BASE_DIR)) {
        std::stringstream ss;
        ss << "The graph db base directory is not configured : " << Constants::GRAPH_BASE_DIR;
        BOOST_THROW_EXCEPTION(keto::account_db::AccountsInvalidDBNameException(
            ss.str()));
    }
    
    // create a db directory
    boost::filesystem::path graphBaseDir =  
        keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / 
        config->getVariablesMap()[Constants::GRAPH_BASE_DIR].as<std::string>();
    
    if (!boost::filesystem::exists(graphBaseDir)) {
        boost::filesystem::create_directory(graphBaseDir);
    }

    boost::filesystem::path dbPath =  graphBaseDir /
                                      dbName;
    if (!boost::filesystem::exists(dbPath)) {
        boost::filesystem::create_directory(dbPath);
    }

    // request the database name
    std::stringstream ss;
    if (dbName == Constants::BASE_GRAPH) {
        ss << "hash-type='bdb',dir='" << dbPath.c_str() << "',write='yes',contexts='yes',index-predicates='yes'";
    } else {
        keto::proto::PasswordRequest passwordRequest;
        passwordRequest.set_identifier(dbName);
        keto::proto::PasswordResponse passwordResponse = keto::server_common::fromEvent<keto::proto::PasswordResponse>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::PasswordRequest>(
                        keto::server_common::Events::REQUEST_PASSWORD, passwordRequest)));
        //KETO_LOG_DEBUG << "The db path is : " << dbPath;
        ss << "hash-type='bdb',dir='" << dbPath.c_str() << "',write='yes',contexts='yes',index-predicates='yes',password='" << passwordResponse.password()
           << "'";
    }

    //KETO_LOG_DEBUG << "The db name is : " << dbName << "[" << ss.str() << "]";
    storage=librdf_new_storage(world, "hashes", dbName.c_str(),
                             ss.str().c_str());
    //KETO_LOG_DEBUG << "Load the model from the storage";
    model = librdf_new_model(world,storage,NULL);
    if (!storage || !model) {
        BOOST_THROW_EXCEPTION(keto::account_db::AccountDBInitFailureException());
    }

    this->storageLockPtr = AccountGraphStore::StorageLockPtr(new AccountGraphStore::StorageLock);
}

AccountGraphStore::~AccountGraphStore() {
    librdf_free_model(model);
    librdf_free_storage(storage);
    librdf_free_world(world);
}


std::string AccountGraphStore::getDbName() {
    return this->dbName;
}


bool AccountGraphStore::checkForDb(const std::string& dbName) {
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::GRAPH_BASE_DIR)) {
        std::stringstream ss;
        ss << "The graph db base directory is not configured : " << Constants::GRAPH_BASE_DIR;
        BOOST_THROW_EXCEPTION(keto::account_db::AccountsInvalidDBNameException(
            ss.str()));
    }
    
    // create a db directory
    boost::filesystem::path graphBaseDir =  
        keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / 
        config->getVariablesMap()[Constants::GRAPH_BASE_DIR].as<std::string>();
    
    if (!boost::filesystem::exists(graphBaseDir)) {
        return false;
    }    
    
    boost::filesystem::path dbPath =  graphBaseDir / 
        dbName;
    if (!boost::filesystem::exists(dbPath)) {
        return false;
    }
    return true;
}


librdf_world* AccountGraphStore::getWorld() {
    return this->world;
}

librdf_storage* AccountGraphStore::getStorage() {
    return this->storage;
}

librdf_model* AccountGraphStore::getModel() {
    return this->model;
}


AccountGraphStore::StorageLockPtr AccountGraphStore::getStorageLock() {
    return this->storageLockPtr;
}

}
}
