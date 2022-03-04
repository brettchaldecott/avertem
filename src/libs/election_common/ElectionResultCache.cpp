//
// Created by Brett Chaldecott on 2019-09-03.
//

#include "keto/election_common/ElectionResultCache.hpp"


namespace keto {
namespace election_common {

std::string ElectionResultCache::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionResultCache::ElectionResultCache() {

}

ElectionResultCache::~ElectionResultCache() {

}

void ElectionResultCache::heartBeat(const keto::software_consensus::ProtocolHeartbeatMessageHelper& protocolHeartbeatMessageHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getNetworkSlot()) {
        this->confirmationCache.clear();
        this->publishCache.clear();
    }
}

bool ElectionResultCache::containsPublishAccount(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    for (keto::election_common::ElectionPublishTangleAccountProtoHelper cacheEntry : publishCache) {
        if (cacheEntry.getAccount() == electionPublishTangleAccountProtoHelper.getAccount()) {
            if (compareVectorHashes(cacheEntry.getTangles(), electionPublishTangleAccountProtoHelper.getTangles())) {
                return true;
            }
        }
    }

    this->publishCache.push_back(electionPublishTangleAccountProtoHelper);
    return false;
}

bool ElectionResultCache::containsConfirmationAccount(const keto::asn1::HashHelper& hashHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->confirmationCache.count(hashHelper)) {
        return true;
    }
    this->confirmationCache.insert(hashHelper);
    return false;
}


bool ElectionResultCache::compareVectorHashes(const std::vector<keto::asn1::HashHelper>& lhs, const std::vector<keto::asn1::HashHelper>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (keto::asn1::HashHelper lhsHash : lhs) {
        bool found = false;
        for (keto::asn1::HashHelper rhsHash : rhs) {
            if (lhsHash == rhsHash) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

}
}