//
// Created by Brett Chaldecott on 2019-09-03.
//

#ifndef KETO_ELECTIONRESULTCACHE_HPP
#define KETO_ELECTIONRESULTCACHE_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <mutex>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/software_consensus/ProtocolHeartbeatMessageHelper.hpp"

namespace keto {
namespace election_common {

class ElectionResultCache;
typedef std::shared_ptr<ElectionResultCache> ElectionResultCachePtr;

class ElectionResultCache {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionResultCache();
    ElectionResultCache(const ElectionResultCache& orig) = delete;
    virtual ~ElectionResultCache();

    void heartBeat(const keto::software_consensus::ProtocolHeartbeatMessageHelper& protocolHeartbeatMessageHelper);
    bool containsPublishAccount(const keto::asn1::HashHelper& hashHelper);
    bool containsConfirmationAccount(const keto::asn1::HashHelper& hashHelper);

private:
    std::mutex classMutex;
    std::set<std::vector<uint8_t>> publishCache;
    std::set<std::vector<uint8_t>> confirmationCache;
};

}
}


#endif //KETO_ELECTIONRESULTCACHE_HPP