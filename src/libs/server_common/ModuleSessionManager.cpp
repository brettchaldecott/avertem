//
// Created by Brett Chaldecott on 2019/04/04.
//

#include "keto/server_common/ModuleSessionManager.hpp"

namespace keto {
namespace server_common {

static ModuleSessionManagerPtr singleton;

std::string ModuleSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ModuleSessionManager::ModuleSessionManager() {

}

ModuleSessionManager::~ModuleSessionManager() {
    this->sessions.clear();
}

ModuleSessionManagerPtr ModuleSessionManager::init() {
    return singleton = ModuleSessionManagerPtr(new ModuleSessionManager());
}

void ModuleSessionManager::fin() {
    singleton.reset();
}

ModuleSessionManagerPtr ModuleSessionManager::getInstance() {
    return singleton;
}

void ModuleSessionManager::addSession(const ModuleSessionPtr& session) {
    if (singleton) {
        singleton->addSessionEntry(session);
    }
}

void ModuleSessionManager::addSessionEntry(const ModuleSessionPtr& session) {
    this->sessions.push_back(session);
}


}
}
