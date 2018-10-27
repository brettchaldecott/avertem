/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RouterStore.hpp
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
#include "keto/router_db/RouterResourceManager.hpp"
#include "keto/obfuscate/MetaString.hpp"

#include "keto/router_utils/PushAccountHelper.hpp"


namespace keto {
namespace router_db {

class RouterStore {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RouterStore();
    RouterStore(const RouterStore& orig) = delete;
    virtual ~RouterStore();
    
    // manage the store
    static std::shared_ptr<RouterStore> init();
    static void fin();
    static std::shared_ptr<RouterStore> getInstance();

    // get the account information
    bool getAccountRouting(
            const keto::asn1::HashHelper& helper,
            keto::proto::AccountRoutingStore& result);
    
    
    void setAccountRouting(
            const keto::asn1::HashHelper& helper,
            const keto::proto::AccountRoutingStore& routing);
    
    void pushAccountRouting(
            const keto::proto::PushAccount& pushAccount);
    
private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    RouterResourceManagerPtr routerResourceManagerPtr;
    
    void pushAccountRouting(
            keto::router_utils::PushAccountHelper& pushAccountHelper);
};


}
}

#endif /* ROUTERSTORE_HPP */

