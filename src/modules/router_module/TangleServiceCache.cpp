//
// Created by Brett Chaldecott on 2019-05-30.
//

#include "keto/router/TangleServiceCache.hpp"


namespace keto {
namespace router {

TangleServiceCache::Service::Service(const std::string& name, const std::string& accountHash) : name(name), accountHash(accountHash) {
    TangleServiceCache::getInstance()->addAccount(accountHash);
}

TangleServiceCache::Service::~Service() {
    TangleServiceCache::getInstance()->removeAccount(accountHash);
}

std::string TangleServiceCache::Service::getName() {
    return this->name;
}

std::string TangleServiceCache::Service::getAccountHash() {
    return this->accountHash;
}


TangleServiceCache::Tangle::Tangle(const std::string& tangle) : tangle(tangle){
}

TangleServiceCache::Tangle::~Tangle() {

}


std::string TangleServiceCache::Tangle::getTangle() {
    return this->tangle;
}

TangleServiceCache::ServicePtr TangleServiceCache::Tangle::getService(const std::string& name) {
    if (services.count(name)) {
        return this->services[name];
    }
    return TangleServiceCache::ServicePtr();
}

TangleServiceCache::ServicePtr TangleServiceCache::Tangle::setService(const std::string& name, const std::string& accountHash) {
    return this->services[name] = TangleServiceCache::ServicePtr(new TangleServiceCache::Service(name,accountHash));
}

std::vector<std::string> TangleServiceCache::Tangle::getServices() {
    std::vector<std::string> results;
    for(std::map<std::string,ServicePtr>::iterator it = this->services.begin(); it != this->services.end(); ++it) {
        results.push_back(it->first);
    }
    return results;
}

static TangleServiceCachePtr singleton;

std::string TangleServiceCache::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TangleServiceCache::TangleServiceCache() {

}

TangleServiceCache::~TangleServiceCache() {

}

TangleServiceCachePtr TangleServiceCache::init() {
    return singleton = TangleServiceCachePtr(new TangleServiceCache());
}

void TangleServiceCache::fin() {
    singleton.reset();
}

TangleServiceCachePtr TangleServiceCache::getInstance() {
    return singleton;
}


TangleServiceCache::TanglePtr TangleServiceCache::addTangle(const std::string& tangle, bool grow) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (grow) {
        return this->growTanglePtr = this->tangles[tangle] = TangleServiceCache::TanglePtr(new TangleServiceCache::Tangle(tangle));
    } else {
        return this->tangles[tangle] = TangleServiceCache::TanglePtr(new TangleServiceCache::Tangle(tangle));
    }
}

TangleServiceCache::TanglePtr TangleServiceCache::getTangle(const std::string& tangle) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    return this->tangles[tangle];
}

TangleServiceCache::TanglePtr TangleServiceCache::getGrowTangle() {
    std::lock_guard<std::mutex> guard(this->classMutex);
    return this->growTanglePtr;
}

void TangleServiceCache::clear() {
    std::lock_guard<std::mutex> guard(this->classMutex);
    this->tangles.clear();
}

bool TangleServiceCache::containsAccount(const std::string& account) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->accounts.count(account)) {
        return true;
    }
    return false;
}

int TangleServiceCache::addAccount(const std::string& account) {
    if (!this->accounts.count(account)) {
        this->accounts[account] = 1;
    } else {
        this->accounts[account]++;
    }
    return this->accounts.size();
}

int TangleServiceCache::removeAccount(const std::string& account) {
    if (!this->accounts.count(account)) {
        return 0;
    }
    int count = this->accounts[account]--;
    if (count <= 0) {
        this->accounts.erase(account);
    }
    return count;
}


}
}