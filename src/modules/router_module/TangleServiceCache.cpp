//
// Created by Brett Chaldecott on 2019-05-30.
//

#include "keto/router/TangleServiceCache.hpp"
#include "keto/router/Exception.hpp"


namespace keto {
namespace router {

std::string TangleServiceCache::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TangleServiceCache::Tangle::Tangle(const keto::asn1::HashHelper& tangle) : tangle(tangle) {
    KETO_LOG_ERROR << "[TangleServiceCache::Tangle::Tangle] The constructor the tangle [" << tangle.getHash(keto::common::StringEncoding::HEX) << "]";
}

TangleServiceCache::Tangle::~Tangle() {

}

keto::asn1::HashHelper TangleServiceCache::Tangle::getTangle() {
    return this->tangle;
}

TangleServiceCache::AccountTangle::AccountTangle(
        const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    this->accountHash = electionPublishTangleAccountProtoHelper.getAccount();
    this->growing = electionPublishTangleAccountProtoHelper.isGrowing();
    for (keto::asn1::HashHelper tangle : electionPublishTangleAccountProtoHelper.getTangles()) {
        TangleServiceCache::TanglePtr tanglePtr(new TangleServiceCache::Tangle(tangle));
        this->tangleList.push_back(tanglePtr);
        this->tangleMap.insert(std::pair<std::string,TangleServiceCache::TanglePtr>(tangle,tanglePtr));
    }
}

TangleServiceCache::AccountTangle::~AccountTangle() {

}


keto::asn1::HashHelper TangleServiceCache::AccountTangle::getFirstTangleHash() {
    return this->tangleList.front()->getTangle();
}

keto::asn1::HashHelper TangleServiceCache::AccountTangle::getAccountHash() {
    return this->accountHash;
}


bool TangleServiceCache::AccountTangle::containsTangle(const keto::asn1::HashHelper& tangle) {
    if (this->tangleMap.count(tangle)) {
        return true;
    }
    return false;
}

TangleServiceCache::TanglePtr TangleServiceCache::AccountTangle::getTangle(const keto::asn1::HashHelper& tangle) {
    return this->tangleMap[tangle];
}

std::vector<keto::asn1::HashHelper> TangleServiceCache::AccountTangle::getTangles() {
    std::vector<keto::asn1::HashHelper> tangles;
    for (TanglePtr tanglePtr: this->tangleList) {
        tangles.push_back(tanglePtr->getTangle());
    }
    return tangles;
}

bool TangleServiceCache::AccountTangle::isGrowing() {
    return this->growing;
}

keto::election_common::ElectionPublishTangleAccountProtoHelperPtr TangleServiceCache::AccountTangle::getElectionPublishTangleAccountProtoHelper() {
    keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelperPtr(
            new keto::election_common::ElectionPublishTangleAccountProtoHelper());
    electionPublishTangleAccountProtoHelperPtr->setAccount(this->accountHash);
    for (TanglePtr tanglePtr: this->tangleList) {
        electionPublishTangleAccountProtoHelperPtr->addTangle(tanglePtr->getTangle());
    }
    electionPublishTangleAccountProtoHelperPtr->setGrowing(this->growing);
    return electionPublishTangleAccountProtoHelperPtr;
}

static TangleServiceCachePtr singleton;

TangleServiceCache::TangleServiceCache() :activeSession(false) {
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

bool TangleServiceCache::containsAccount(const keto::asn1::HashHelper& account) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->sessionAccounts.count(account)) {
        return true;
    }
    return false;
}

bool TangleServiceCache::containTangle(const keto::asn1::HashHelper& tangle) {
    std::lock_guard<std::mutex> guard(classMutex);
    for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
        if (iter->second->containsTangle(tangle)) {
            return true;
        }
    }
    return false;
}

TangleServiceCache::AccountTanglePtr TangleServiceCache::getGrowing() {
    std::lock_guard<std::mutex> guard(classMutex);
    for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
        if (iter->second->isGrowing()) {
            return iter->second;
        }
    }
    BOOST_THROW_EXCEPTION(NoGrowingTangle());
}

TangleServiceCache::AccountTanglePtr TangleServiceCache::getTangle(const keto::asn1::HashHelper& tangle) {
    std::lock_guard<std::mutex> guard(classMutex);
    for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
        if (iter->second->containsTangle(tangle)) {
            return iter->second;
        }
    }
    std::stringstream ss;
    ss << "Cannot find the tangle [" << tangle.getHash(keto::common::StringEncoding::HEX) << "][" << this->sessionAccounts.size() << "]";
    BOOST_THROW_EXCEPTION(NoMatchingTangleFound(ss.str()));
}

void TangleServiceCache::publish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    this->nextSessionAccounts[electionPublishTangleAccountProtoHelper.getAccount()] =
            AccountTanglePtr(new AccountTangle(electionPublishTangleAccountProtoHelper));
}


void TangleServiceCache::confirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    this->activeSession = true;
    AccountTanglePtr accountTanglePtr = this->nextSessionAccounts[electionConfirmationHelper.getAccount()];
    if (!accountTanglePtr) {
        this->nextSessionAccounts.erase(electionConfirmationHelper.getAccount());
        KETO_LOG_ERROR << "[TangleServiceCache::confirmation]Failed to find the account [" << electionConfirmationHelper.getAccount().getHash(keto::common::StringEncoding::HEX) << "]";
        return;
    }
    // copy the session accounts that need to be copied
    std::map<std::string,AccountTanglePtr> sessionAccounts;
    for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
        if (iter->second && !iter->second->containsTangle(accountTanglePtr->getFirstTangleHash())) {
            KETO_LOG_INFO << "[TangleServiceCache::confirmation] Found previous tangle manager[" << iter->second->getAccountHash().getHash(keto::common::StringEncoding::HEX)
                          << "] removing it";
            sessionAccounts[iter->first] = iter->second;
        }
    }
    sessionAccounts[electionConfirmationHelper.getAccount()] = accountTanglePtr;
    this->sessionAccounts = sessionAccounts;
    this->nextSessionAccounts.erase(electionConfirmationHelper.getAccount());
    KETO_LOG_INFO << "[TangleServiceCache::confirmation] Added the account [" << electionConfirmationHelper.getAccount().getHash(keto::common::StringEncoding::HEX)
        << "] to the tangle cache list";
}

keto::election_common::PublishedElectionInformationHelperPtr TangleServiceCache::getPublishedElection() {
    std::lock_guard<std::mutex> guard(classMutex);
    keto::election_common::PublishedElectionInformationHelperPtr publishedElectionInformationHelperPtr(new keto::election_common::PublishedElectionInformationHelper());

    for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
        publishedElectionInformationHelperPtr->addElectionPublishTangleAccount(iter->second->getElectionPublishTangleAccountProtoHelper());
    }

    return publishedElectionInformationHelperPtr;
}

void TangleServiceCache::setPublishedElection(const keto::election_common::PublishedElectionInformationHelperPtr& publishedElectionInformationHelperPtr) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->activeSession) {
        return;
    }
    for (keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelperPtr : publishedElectionInformationHelperPtr->getElectionPublishTangleAccounts()) {
        this->sessionAccounts[electionPublishTangleAccountProtoHelperPtr->getAccount()] = AccountTanglePtr(new AccountTangle(*electionPublishTangleAccountProtoHelperPtr));
    }
    this->activeSession = true;
}

keto::chain_query_common::ProducerResultProtoHelper TangleServiceCache::getProducers() {
    std::lock_guard<std::mutex> guard(classMutex);
    keto::chain_query_common::ProducerResultProtoHelper result;
    for (std::map<std::string,AccountTanglePtr>::iterator iter = sessionAccounts.begin();
            iter != sessionAccounts.end(); iter++) {
        keto::chain_query_common::ProducerInfoResultProtoHelper producerInfoResultProtoHelper;
        producerInfoResultProtoHelper.setAccountHashId(iter->second->getAccountHash());
        producerInfoResultProtoHelper.setTangles(iter->second->getTangles());
        result.addProducer(producerInfoResultProtoHelper);
    }
    return result;
}

}
}