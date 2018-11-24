/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RouterResource.hpp
 * Author: ubuntu
 *
 * Created on February 28, 2018, 10:56 AM
 */

#ifndef ROUTER_RESOURCE_HPP
#define ROUTER_RESOURCE_HPP

#include <string>
#include <memory>
#include <map>

#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

#include "keto/transaction/Resource.hpp"

#include "keto/rocks_db/DBManager.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace router_db {

class RouterResource;
typedef std::shared_ptr<RouterResource> RouterResourcePtr;

class RouterResource {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RouterResource(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr);
    RouterResource(const RouterResource& orig) = delete;
    virtual ~RouterResource();
    
    void commit();
    void rollback();
    
    
    rocksdb::Transaction* getTransaction(const std::string& name);
    
private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    std::map<std::string,rocksdb::Transaction*> transactionMap;
    
};


}
}
#endif /* BLOCKRESOURCE_HPP */

