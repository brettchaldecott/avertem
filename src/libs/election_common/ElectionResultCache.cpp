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

bool ElectionResultCache::containsPublishAccount(const keto::asn1::HashHelper& hashHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->confirmationCache.count(hashHelper)) {
        return true;
    }
    this->confirmationCache.insert(hashHelper);
    return false;
}

bool ElectionResultCache::containsConfirmationAccount(const keto::asn1::HashHelper& hashHelper) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->publishCache.count(hashHelper)) {
        return true;
    }
    this->publishCache.insert(hashHelper);
    return false;
}

}
}