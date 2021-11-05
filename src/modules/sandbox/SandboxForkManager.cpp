//
// Created by Brett Chaldecott on 2021/11/04.
//

#include "keto/sandbox/SandboxForkManager.hpp"

namespace keto {
namespace sandbox {

static SandboxForkManagerPtr singleton;

std::string SandboxForkManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


SandboxForkManager::SandboxForkWrapper::SandboxForkWrapper(const SandboxForkPtr sandboxForkPtr) : sandboxForkPtr(sandboxForkPtr){
    this->sandboxForkPtr->incrementUsageCount();
}

SandboxForkManager::SandboxForkWrapper::~SandboxForkWrapper() {
    SandboxForkManager::getInstance()->releaseFork(this->sandboxForkPtr);
}

keto::event::Event SandboxForkManager::SandboxForkWrapper::executeActionMessage(const keto::event::Event& event) {
    return this->sandboxForkPtr->executeActionMessage(event);
}

keto::event::Event SandboxForkManager::SandboxForkWrapper::executeHttpActionMessage(const keto::event::Event& event) {
    return this->sandboxForkPtr->executeHttpActionMessage(event);
}

SandboxForkManager::SandboxForkManager() {

}
SandboxForkManager::~SandboxForkManager() {

}

SandboxForkManagerPtr SandboxForkManager::init() {
    if (!singleton) {
        singleton = SandboxForkManagerPtr(new SandboxForkManager);
    }
    return singleton;
}

void SandboxForkManager::fin() {
    if (singleton) {
        singleton->terminate();
        singleton.reset();
    }
}

SandboxForkManagerPtr SandboxForkManager::getInstance() {
    return singleton;
}

SandboxForkManager::SandboxForkWrapperPtr SandboxForkManager::getFork() {
    KETO_LOG_INFO << "[SandboxForkManager::getFork] Get a fork";
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (!this->sandboxForkDequeu.empty()) {
        SandboxForkManager::SandboxForkWrapperPtr sandboxForkWrapperPtr(new SandboxForkWrapper(this->sandboxForkDequeu.front()));
        this->sandboxForkDequeu.pop_front();
        KETO_LOG_INFO << "[SandboxForkManager::getFork] Return an existing fork";
        return sandboxForkWrapperPtr;
    } else {
        SandboxForkManager::SandboxForkWrapperPtr sandboxForkWrapperPtr(new SandboxForkWrapper(
                SandboxForkPtr(new SandboxFork())));
        this->forkCount++;
        KETO_LOG_INFO << "[SandboxForkManager::getFork] Return a new fork";
        return sandboxForkWrapperPtr;
    }
}

void SandboxForkManager::releaseFork(const SandboxForkPtr& sandboxForkPtr) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->sandboxForkDequeu.push_back(sandboxForkPtr);
    this->stateCondition.notify_all();
}

void SandboxForkManager::terminate() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    while(this->forkCount > 0) {
        if (this->sandboxForkDequeu.empty()) {
            this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(1));
            continue;
        }
        SandboxForkPtr sandboxForkPtr = this->sandboxForkDequeu.front();
        sandboxForkPtr->terminate();
        this->sandboxForkDequeu.pop_front();
        this->forkCount--;
    }
}

}
}