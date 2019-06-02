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

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"


namespace keto {
namespace router {

class TangleServiceCache;
typedef std::shared_ptr<TangleServiceCache> TangleServiceCachePtr;

class TangleServiceCache {
public:
    class Service {
    public:
        Service(const std::string& name, const std::string& accountHash);
        Service(const Service& orig) = delete;
        virtual ~Service();

        std::string getName();
        std::string getAccountHash();

    private:
        std::string name;
        std::string accountHash;

    };
    typedef std::shared_ptr<Service> ServicePtr;

    class Tangle {
    public:
        Tangle(const std::string& tangle);
        Tangle(const Tangle& orig) = delete;
        virtual ~Tangle();

        std::string getTangle();
        ServicePtr getService(const std::string& name);
        ServicePtr setService(const std::string& name, const std::string& accountHash);
        std::vector<std::string> getServices();

    private:
        std::string tangle;
        std::map<std::string,ServicePtr> services;


    };
    typedef std::shared_ptr<Tangle> TanglePtr;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    TangleServiceCache();
    TangleServiceCache(const TangleServiceCache& orig) = delete;
    virtual ~TangleServiceCache();

    static TangleServiceCachePtr init();
    static void fin();
    static TangleServiceCachePtr getInstance();


    TanglePtr addTangle(const std::string& tangle, bool grow = false);
    TanglePtr getTangle(const std::string& tangle);
    TanglePtr getGrowTangle();
    void clear();

    bool containsAccount(const std::string& account);
protected:
    int addAccount(const std::string& account);
    int removeAccount(const std::string& account);
private:
    std::mutex classMutex;
    std::map<std::string,TanglePtr> tangles;
    TanglePtr growTanglePtr;
    std::map<std::string,int> accounts;

};


}
}


#endif //KETO_TANGLESERVICECACHE_HPP
