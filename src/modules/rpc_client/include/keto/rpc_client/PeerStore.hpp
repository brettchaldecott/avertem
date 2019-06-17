/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerStore.hpp
 * Author: ubuntu
 *
 * Created on March 6, 2018, 3:20 AM
 */

#ifndef ROUTERSTORE_HPP
#define ROUTERSTORE_HPP

#include <string>
#include <memory>

#include "Route.pb.h"

#include "keto/rocks_db/DBManager.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rpc_client/PeerResourceManager.hpp"
#include "keto/obfuscate/MetaString.hpp"

#include "keto/router_utils/PushAccountHelper.hpp"


namespace keto {
namespace rpc_client {

class PeerStore;
typedef std::shared_ptr<PeerStore> PeerStorePtr;

class PeerStore {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    PeerStore();
    PeerStore(const PeerStore& orig) = delete;
    virtual ~PeerStore();
    
    // manage the store
    static PeerStorePtr init();
    static void fin();
    static PeerStorePtr getInstance();

    // get the peers
    void setPeers(const std::vector<std::string>& peers);
    std::vector<std::string> getPeers();
    
private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    PeerResourceManagerPtr peerResourceManagerPtr;
    
    void pushAccountRouting(
            keto::router_utils::PushAccountHelper& pushAccountHelper);
};


}
}

#endif /* ROUTERSTORE_HPP */

