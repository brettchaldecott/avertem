/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VersionProcessor.cpp
 * Author: ubuntu
 * 
 * Created on September 28, 2018, 3:40 PM
 */

#include <sstream>

#include "keto/version_manager/VersionProcessor.hpp"

#include "keto/common/Log.hpp"

#include "keto/version_manager/Constants.hpp"
#include "keto/version_manager/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"
#include "include/keto/version_manager/VersionProcessor.hpp"


namespace keto {
namespace version_manager {

static VersionProcessorPtr singleton; 
static std::shared_ptr<std::thread> versionThreadPtr; 


std::string VersionProcessor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

    
VersionProcessor::VersionProcessor() {
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    
    ketoHome = std::getenv("KETO_HOME");    
    
    if (!config->getVariablesMap().count(Constants::AUTO_UPDATE)) {
        BOOST_THROW_EXCEPTION(keto::version_manager::AutoUpdateNotConfiguredException());
    }
    
    std::string autoUpdate = 
        config->getVariablesMap()[Constants::AUTO_UPDATE].as<std::string>();
    KETO_LOG_INFO << "[VersionProcess] auto update : " << autoUpdate;
    this->autoUpdate = true;
    if (autoUpdate.compare("false") == 0) {
        this->autoUpdate = false;
    }
    
    if (this->autoUpdate && !config->getVariablesMap().count(Constants::CHECK_SCRIPT)) {
        BOOST_THROW_EXCEPTION(keto::version_manager::CheckScriptNotConfiguredException());
    } else if (this->autoUpdate && config->getVariablesMap().count(Constants::CHECK_SCRIPT)) {
        std::stringstream sstream;
        sstream << ketoHome << "/" << 
            config->getVariablesMap()[Constants::CHECK_SCRIPT].as<std::string>();
        this->checkScript = sstream.str();
    }
}

VersionProcessor::~VersionProcessor() {
}

VersionProcessorPtr VersionProcessor::init() {
    singleton = std::make_shared<VersionProcessor>();
    if (singleton->isAutoUpdateEnabled()) {
    versionThreadPtr = std::shared_ptr<std::thread>(new std::thread(
        []
        {
            singleton->run();
        }));
    }
    return singleton;
}

void VersionProcessor::fin() {
    if (singleton->isAutoUpdateEnabled()) {
        singleton->terminate();
        versionThreadPtr->join();
        versionThreadPtr.reset();
    }
    singleton.reset();
}

VersionProcessorPtr VersionProcessor::getInstance() {
    return singleton;
}

void VersionProcessor::run() {
    while(!this->checkTerminated()) {
        KETO_LOG_INFO << "[VersionProcess] perform check";
        performUpdateCheck();
        KETO_LOG_INFO << "[VersionProcess] perform update check";
    }
}

void VersionProcessor::terminate() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->terminated = true;
    this->stateCondition.notify_all();
}

bool VersionProcessor::isTerminated() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->terminated;
}

bool VersionProcessor::isAutoUpdateEnabled() {
    return this->autoUpdate;
}


bool VersionProcessor::checkTerminated() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->stateCondition.wait_for(uniqueLock,std::chrono::milliseconds(90 * 60 * 1000));
    return this->terminated;
}


void VersionProcessor::performUpdateCheck() {
    std::system(this->checkScript.c_str());
}

}
}