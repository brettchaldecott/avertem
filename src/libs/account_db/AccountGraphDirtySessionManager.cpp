//
// Created by Brett Chaldecott on 2019/03/04.
//

#include "keto/account_db/AccountGraphDirtySessionManager.hpp"


namespace keto {
namespace account_db {

static AccountGraphDirtySessionManagerPtr singleton;

std::string AccountGraphDirtySessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountGraphDirtySessionManager::AccountGraphDirtySessionManager() {

}

AccountGraphDirtySessionManager::~AccountGraphDirtySessionManager() {

}

AccountGraphDirtySessionManagerPtr AccountGraphDirtySessionManager::init() {
    return singleton = AccountGraphDirtySessionManagerPtr(new AccountGraphDirtySessionManager());
}

AccountGraphDirtySessionManagerPtr AccountGraphDirtySessionManager::getInstance() {
    return singleton;
}

void AccountGraphDirtySessionManager::fin() {
    singleton.reset();
}

AccountGraphDirtySessionPtr AccountGraphDirtySessionManager::getDirtySession(const std::string& name) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    if (this->dirtySessions.count(name)) {
        return this->dirtySessions[name];
    }
    AccountGraphDirtySessionPtr accountGraphDirtySessionPtr(new AccountGraphDirtySession(name));
    this->dirtySessions[name] = accountGraphDirtySessionPtr;
    return accountGraphDirtySessionPtr;
}

void AccountGraphDirtySessionManager::clearSessions() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->dirtySessions.clear();
}

}
}