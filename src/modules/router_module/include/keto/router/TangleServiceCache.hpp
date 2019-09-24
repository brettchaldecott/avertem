//
// Created by Brett Chaldecott on 2019-05-30.
//

#ifndef KETO_TANGLESERVICECACHE_HPP
#define KETO_TANGLESERVICECACHE_HPP

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>


#include "Protocol.pb.h"

#include "keto/asn1/HashHelper.hpp"

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"

#include "keto/chain_query_common/ProducerResultProtoHelper.hpp"


namespace keto {
namespace router {

class TangleServiceCache;
typedef std::shared_ptr<TangleServiceCache> TangleServiceCachePtr;

class TangleServiceCache {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class Tangle {
    public:
        Tangle(const keto::asn1::HashHelper& tangle);
        Tangle(const Tangle& orig) = delete;
        virtual ~Tangle();

        keto::asn1::HashHelper getTangle();
    private:
        keto::asn1::HashHelper tangle;


    };
    typedef std::shared_ptr<Tangle> TanglePtr;

    class AccountTangle {
    public:
        AccountTangle(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper);
        AccountTangle(const AccountTangle& accountTangle) = delete;
        virtual ~AccountTangle();

        keto::asn1::HashHelper getFirstTangleHash();
        keto::asn1::HashHelper getAccountHash();
        bool containsTangle(const keto::asn1::HashHelper& tangle);
        TanglePtr getTangle(const keto::asn1::HashHelper& tangle);
        std::vector<keto::asn1::HashHelper> getTangles();
        bool isGrowing();

    private:
        keto::asn1::HashHelper accountHash;
        bool growing;
        std::vector<TanglePtr> tangleList;
        std::map<std::string,TanglePtr> tangleMap;

    };
    typedef std::shared_ptr<AccountTangle> AccountTanglePtr;


    TangleServiceCache();
    TangleServiceCache(const TangleServiceCache& orig) = delete;
    virtual ~TangleServiceCache();

    static TangleServiceCachePtr init();
    static void fin();
    static TangleServiceCachePtr getInstance();

    bool containsAccount(const keto::asn1::HashHelper& account);
    bool containTangle(const keto::asn1::HashHelper& tangle);
    AccountTanglePtr getGrowing();
    AccountTanglePtr getTangle(const keto::asn1::HashHelper& tangle);

    void publish(const keto::election_common::ElectionPublishTangleAccountProtoHelper& electionPublishTangleAccountProtoHelper);
    void confirmation(const keto::election_common::ElectionConfirmationHelper& electionPublishTangleAccountProtoHelper);

    keto::chain_query_common::ProducerResultProtoHelper getProducers();

private:
    std::mutex classMutex;
    std::map<std::string,AccountTanglePtr> sessionAccounts;
    std::map<std::string,AccountTanglePtr> nextSessionAccounts;


};


}
}


#endif //KETO_TANGLESERVICECACHE_HPP
