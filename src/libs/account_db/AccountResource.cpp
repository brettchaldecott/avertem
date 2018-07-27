/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountResource.cpp
 * Author: ubuntu
 * 
 * Created on February 28, 2018, 10:56 AM
 */

#include <rocksdb/utilities/transaction.h>

#include "keto/account_db/AccountResource.hpp"

namespace keto {
namespace account_db {

std::string AccountResource::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

AccountResource::AccountResource(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
        const AccountGraphStoreManagerPtr& accountGraphStoreManagerPtr) : 
dbManagerPtr(dbManagerPtr),accountGraphStoreManagerPtr(accountGraphStoreManagerPtr)  {
}

AccountResource::~AccountResource() {
    // rollback all changes as we can assume the resource is not getting cleared correctly
    for(std::map<std::string,rocksdb::Transaction*>::iterator iter = 
            transactionMap.begin(); iter != transactionMap.end(); iter++)
    {
        iter->second->Rollback();
        delete iter->second;
    }
    transactionMap.clear();
}

void AccountResource::commit() {
    for(std::map<std::string,rocksdb::Transaction*>::iterator iter = 
            transactionMap.begin(); iter != transactionMap.end(); iter++)
    {
        iter->second->Commit();
        delete iter->second;
    }
    transactionMap.clear();
    for(std::map<std::string,AccountGraphSessionPtr>::iterator iter = 
            sessionMap.begin(); iter != sessionMap.end(); iter++)
    {
        iter->second->commit();
    }
    sessionMap.clear();
}

void AccountResource::rollback() {
    // rollback all changes as we can assume the resource is not getting cleared correctly
    for(std::map<std::string,rocksdb::Transaction*>::iterator iter = 
            transactionMap.begin(); iter != transactionMap.end(); iter++)
    {
        iter->second->Rollback();
        delete iter->second;
    }
    transactionMap.clear();
    for(std::map<std::string,AccountGraphSessionPtr>::iterator iter = 
            sessionMap.begin(); iter != sessionMap.end(); iter++)
    {
        iter->second->rollback();
    }
    sessionMap.clear();
}


rocksdb::Transaction* AccountResource::getTransaction(const std::string& name) {
    if (!transactionMap.count(name)) {
        keto::rocks_db::DBConnectorPtr dbConnectionPtr = 
                dbManagerPtr->getConnection(name);
        rocksdb::WriteOptions write_options;
        transactionMap[name] = dbConnectionPtr->getDB()->BeginTransaction(
                write_options);
        
    }
    return transactionMap[name];
}

AccountGraphSessionPtr AccountResource::getGraphSession(const std::string& name) {
    if (!this->sessionMap.count(name)) {
        AccountGraphStorePtr accountGraphStorePtr = (*this->accountGraphStoreManagerPtr)[name];
        this->sessionMap[name] = AccountGraphSessionPtr(
                new AccountGraphSession(accountGraphStorePtr));
    }
    return this->sessionMap[name];
}

}
}