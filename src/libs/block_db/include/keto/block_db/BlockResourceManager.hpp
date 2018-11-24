/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockResourceManager.hpp
 * Author: ubuntu
 *
 * Created on February 28, 2018, 11:01 AM
 */

#ifndef BLOCKRESOURCEMANAGER_HPP
#define BLOCKRESOURCEMANAGER_HPP

#include <string>
#include <memory>

#include "keto/transaction/Resource.hpp"

#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResource.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace block_db {

class BlockResourceManager;
typedef std::shared_ptr<BlockResourceManager> BlockResourceManagerPtr;

    
class BlockResourceManager : keto::transaction::Resource {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockResourceManager(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr);
    BlockResourceManager(const BlockResourceManager& orig) = delete;
    virtual ~BlockResourceManager();
    
    virtual void commit();
    virtual void rollback();
    
    BlockResourcePtr getResource();
    
private:
    static thread_local BlockResourcePtr blockResourcePtr;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
};


}
}

#endif /* BLOCKRESOURCEMANAGER_HPP */

