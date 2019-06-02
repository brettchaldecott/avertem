/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerResourceManager.hpp
 * Author: ubuntu
 *
 * Created on February 28, 2018, 11:01 AM
 */

#ifndef RPCCLIENT_RESOURCEMANAGER_HPP
#define RPCCLIENT_RESOURCEMANAGER_HPP

#include <string>
#include <memory>

#include "keto/transaction/Resource.hpp"

#include "keto/rocks_db/DBManager.hpp"
#include "keto/rpc_client/PeerResource.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace rpc_client {

class PeerResourceManager;
typedef std::shared_ptr<PeerResourceManager> PeerResourceManagerPtr;

    
class PeerResourceManager : keto::transaction::Resource {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    PeerResourceManager(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr);
    PeerResourceManager(const PeerResourceManager& orig) = delete;
    virtual ~PeerResourceManager();
    
    virtual void commit();
    virtual void rollback();
    
    PeerResourcePtr getResource();
    
private:
    static thread_local PeerResourcePtr peerResourcePtr;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
};


}
}

#endif /* BLOCKRESOURCEMANAGER_HPP */

