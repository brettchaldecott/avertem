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

TangleServiceCache::TangleWindow::TangleWindow(
        const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    this->accountHash = electionPublishTangleAccountProtoHelper.getAccount();
    this->growing = electionPublishTangleAccountProtoHelper.isGrowing();
    if (this->growing) {
        KETO_LOG_INFO << "[TangleServiceCache::TangleWindow::TangleWindow] The tangle is configured as the growing tangle [" <<
            accountHash.getHash(keto::common::StringEncoding::HEX) << "]";
    }
    for (keto::asn1::HashHelper tangle : electionPublishTangleAccountProtoHelper.getTangles()) {
        TangleServiceCache::TanglePtr tanglePtr(new TangleServiceCache::Tangle(tangle));
        this->tangleList.push_back(tanglePtr);
        this->tangleMap.insert(std::pair<std::string,TangleServiceCache::TanglePtr>(tangle,tanglePtr));
    }
}

TangleServiceCache::TangleWindow::~TangleWindow() {

}

keto::asn1::HashHelper TangleServiceCache::TangleWindow::getFirstTangleHash() {
    return this->tangleList.front()->getTangle();
}

keto::asn1::HashHelper TangleServiceCache::TangleWindow::getAccountHash() {
    return this->accountHash;
}

bool TangleServiceCache::TangleWindow::containsTangle(const keto::asn1::HashHelper& tangle) {
    if (this->tangleMap.count(tangle)) {
        return true;
    }
    return false;
}

TangleServiceCache::TanglePtr TangleServiceCache::TangleWindow::getTangle(const keto::asn1::HashHelper& tangle) {
    return this->tangleMap[tangle];
}

std::vector<keto::asn1::HashHelper> TangleServiceCache::TangleWindow::getTangles() {
    std::vector<keto::asn1::HashHelper> tangles;
    for (TanglePtr tanglePtr: this->tangleList) {
        tangles.push_back(tanglePtr->getTangle());
    }
    return tangles;
}

bool TangleServiceCache::TangleWindow::isGrowing() {
    return this->growing;
}

keto::election_common::ElectionPublishTangleAccountProtoHelperPtr TangleServiceCache::TangleWindow::getElectionPublishTangleAccountProtoHelper() {
    keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelperPtr(
            new keto::election_common::ElectionPublishTangleAccountProtoHelper());
    electionPublishTangleAccountProtoHelperPtr->setAccount(this->accountHash);
    for (TanglePtr tanglePtr: this->tangleList) {
        electionPublishTangleAccountProtoHelperPtr->addTangle(tanglePtr->getTangle());
    }
    electionPublishTangleAccountProtoHelperPtr->setGrowing(this->growing);
    return electionPublishTangleAccountProtoHelperPtr;
}

bool TangleServiceCache::TangleWindow::compareWindow(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    if (!(this->accountHash == electionPublishTangleAccountProtoHelper.getAccount())) {
        return false;
    }
    for (keto::asn1::HashHelper tangle : electionPublishTangleAccountProtoHelper.getTangles()) {
        if (!this->tangleMap.count(tangle)) {
            return false;
        }
    }
    return true;
}

TangleServiceCache::AccountTangle::AccountTangle(
        const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    this->accountHash = electionPublishTangleAccountProtoHelper.getAccount();
    this->tangleWindowList.push_back(TangleWindowPtr(new TangleWindow(electionPublishTangleAccountProtoHelper)));
}

TangleServiceCache::AccountTangle::~AccountTangle() {

}

void TangleServiceCache::AccountTangle::addElectionResults(
        const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {

    for (TangleWindowPtr tangleWindowPtr: this->tangleWindowList) {
        if (tangleWindowPtr->compareWindow(electionPublishTangleAccountProtoHelper)){
            return;
        }
    }
    this->tangleWindowList.push_back(TangleWindowPtr(new TangleWindow(electionPublishTangleAccountProtoHelper)));
}

keto::asn1::HashHelper TangleServiceCache::AccountTangle::getAccountHash() {
    return this->accountHash;
}


bool TangleServiceCache::AccountTangle::containsTangle(const keto::asn1::HashHelper& tangle) {
    for (TangleWindowPtr tangleWindowPtr: this->tangleWindowList) {
        if (tangleWindowPtr->containsTangle(tangle)){
            return true;
        }
    }
    return false;
}

TangleServiceCache::TanglePtr TangleServiceCache::AccountTangle::getTangle(const keto::asn1::HashHelper& tangle) {
    for (TangleWindowPtr tangleWindowPtr: this->tangleWindowList) {
        if (tangleWindowPtr->containsTangle(tangle)){
            return tangleWindowPtr->getTangle(tangle);
        }
    }
    return TangleServiceCache::TanglePtr();
}

std::vector<keto::asn1::HashHelper> TangleServiceCache::AccountTangle::getTangles() {
    std::vector<keto::asn1::HashHelper> tangles;
    for (TangleWindowPtr tangleWindowPtr: this->tangleWindowList) {
        return tangleWindowPtr->getTangles();
    }
    return tangles;
}

bool TangleServiceCache::AccountTangle::isGrowing() {
    for (TangleWindowPtr tangleWindowPtr: this->tangleWindowList) {
        if (tangleWindowPtr->isGrowing()) {
            return true;
        }
    }
    return false;
}

std::vector<keto::election_common::ElectionPublishTangleAccountProtoHelperPtr> TangleServiceCache::AccountTangle::getElectionPublishTangleAccountProtoHelper() {
    std::vector<keto::election_common::ElectionPublishTangleAccountProtoHelperPtr> result;
    for (TangleWindowPtr tangleWindowPtr: this->tangleWindowList) {
        result.push_back(tangleWindowPtr->getElectionPublishTangleAccountProtoHelper());
    }
    return result;
}

static TangleServiceCachePtr singleton;

TangleServiceCache::TangleServiceCache() :activeSession(false), confirmedTime(0) {
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
        for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
            KETO_LOG_INFO << "[TangleServiceCache::containsAccount] session account [" << asn1::HashHelper(iter->first).getHash(
                    keto::common::StringEncoding::HEX) << "] was found in tangle cache.";
        }
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
    // deactivate the active session flag as a publish is now being processed
    if (this->activeSession) {
        this->activeSession = false;
        this->nextSessionAccounts.clear();
    }
    // check confirmation window
    time_t confirmationCheckWindow = time(0) - 120;
    if (!(this->confirmedTime < confirmationCheckWindow)) {
        return;
    }

    if (this->nextSessionAccounts.count(electionPublishTangleAccountProtoHelper.getAccount())) {
        KETO_LOG_INFO << "[TangleServiceCache::publish] Add to the election results list ["
        << electionPublishTangleAccountProtoHelper.getAccount().getHash(keto::common::StringEncoding::HEX) << "]";
        this->nextSessionAccounts[electionPublishTangleAccountProtoHelper.getAccount()]->
            addElectionResults(electionPublishTangleAccountProtoHelper);
    } else {
        KETO_LOG_INFO << "[TangleServiceCache::publish] Add a new election result ["
        << electionPublishTangleAccountProtoHelper.getAccount().getHash(keto::common::StringEncoding::HEX) << "]";
        this->nextSessionAccounts[electionPublishTangleAccountProtoHelper.getAccount()] =
                AccountTanglePtr(new AccountTangle(electionPublishTangleAccountProtoHelper));
    }
}


void TangleServiceCache::confirmation(const keto::election_common::ElectionConfirmationHelper& electionConfirmationHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    // check to see we have not received an echo or if the list has not already been confirmed
    AccountTanglePtr accountTanglePtr = this->nextSessionAccounts[electionConfirmationHelper.getAccount()];
    if (!accountTanglePtr) {
        this->nextSessionAccounts.erase(electionConfirmationHelper.getAccount());
        KETO_LOG_INFO << "[TangleServiceCache::confirmation] assuming window has been configure ignoring account ["
            << electionConfirmationHelper.getAccount().getHash(keto::common::StringEncoding::HEX) << "]";
        return;
    }
    // override the session accounts with the next list on the first confirmation.
    if (this->nextSessionAccounts.size()) {
        KETO_LOG_INFO << "[TangleServiceCache::confirmation] override the session accounts with ["
            << this->nextSessionAccounts.size() << "] was [" << this->sessionAccounts.size() << "]";
        this->activeSession = true;
        this->confirmedTime = time(0);
        this->sessionAccounts = this->nextSessionAccounts;
        this->nextSessionAccounts.clear();
    }
    // copy the session accounts that need to be copied
    //std::map<std::string,AccountTanglePtr> sessionAccounts;
    //for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
    //    if (iter->second && !iter->second->containsTangle(accountTanglePtr->getFirstTangleHash())) {
    //        KETO_LOG_INFO << "[TangleServiceCache::confirmation] Found previous tangle manager[" << iter->second->getAccountHash().getHash(keto::common::StringEncoding::HEX)
    //                      << "] removing it";
    //        sessionAccounts[iter->first] = iter->second;
    //    }
    //}
    //sessionAccounts[electionConfirmationHelper.getAccount()] = accountTanglePtr;
    //this->sessionAccounts = sessionAccounts;
    //this->nextSessionAccounts.erase(electionConfirmationHelper.getAccount());
    KETO_LOG_INFO << "[TangleServiceCache::confirmation] Added the account [" << electionConfirmationHelper.getAccount().getHash(keto::common::StringEncoding::HEX)
        << "] to the tangle cache list";
}

keto::election_common::PublishedElectionInformationHelperPtr TangleServiceCache::getPublishedElection() {
    std::lock_guard<std::mutex> guard(classMutex);
    keto::election_common::PublishedElectionInformationHelperPtr publishedElectionInformationHelperPtr(new keto::election_common::PublishedElectionInformationHelper());

    for (std::map<std::string, AccountTanglePtr>::iterator iter = this->sessionAccounts.begin(); iter != this->sessionAccounts.end(); iter++) {
        for (keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelper : iter->second->getElectionPublishTangleAccountProtoHelper()) {
            publishedElectionInformationHelperPtr->addElectionPublishTangleAccount(electionPublishTangleAccountProtoHelper);
        }
    }

    return publishedElectionInformationHelperPtr;
}

void TangleServiceCache::setPublishedElection(const keto::election_common::PublishedElectionInformationHelperPtr& publishedElectionInformationHelperPtr) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->activeSession) {
        return;
    }
    for (keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelperPtr : publishedElectionInformationHelperPtr->getElectionPublishTangleAccounts()) {
        if (this->sessionAccounts.count(electionPublishTangleAccountProtoHelperPtr->getAccount())) {
            KETO_LOG_INFO << "[TangleServiceCache::setPublishedElection] Add to the election results list ["
            << electionPublishTangleAccountProtoHelperPtr->getAccount().getHash(keto::common::StringEncoding::HEX) << "]";
            this->sessionAccounts[electionPublishTangleAccountProtoHelperPtr->getAccount()]->
            addElectionResults(*electionPublishTangleAccountProtoHelperPtr);
        } else {
            KETO_LOG_INFO << "[TangleServiceCache::setPublishedElection] Add a new election result ["
            << electionPublishTangleAccountProtoHelperPtr->getAccount().getHash(keto::common::StringEncoding::HEX) << "]";
            this->sessionAccounts[electionPublishTangleAccountProtoHelperPtr->getAccount()] =
                    AccountTanglePtr(new AccountTangle(*electionPublishTangleAccountProtoHelperPtr));
        }
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