/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerCache.hpp
 * Author: ubuntu
 *
 * Created on August 25, 2018, 3:14 PM
 */

#ifndef PEERCACHE_HPP
#define PEERCACHE_HPP

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "Protocol.pb.h"

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

namespace keto {
namespace router {

class PeerCache;
typedef std::shared_ptr<PeerCache> PeerCachePtr;

class PeerCache {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    PeerCache();
    PeerCache(const PeerCache& orig) = delete;
    virtual ~PeerCache();
    
    static PeerCachePtr init();
    static void fin();
    static PeerCachePtr getInstance();
    
    void addPeer(keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    void removePeer(keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    keto::router_utils::RpcPeerHelper& getPeer(
            const std::vector<uint8_t>& accountHash);
    bool contains(const std::vector<uint8_t>& accountHash);
    std::vector<uint8_t> electPeer(const std::vector<uint8_t>& accountHash);
    
private:
    std::mutex classMutex;
    std::map<std::vector<uint8_t>,keto::router_utils::RpcPeerHelper> entries;


    std::vector<std::vector<uint8_t>> getAccounts();
};


}
}

#endif /* PEERCACHE_HPP */

