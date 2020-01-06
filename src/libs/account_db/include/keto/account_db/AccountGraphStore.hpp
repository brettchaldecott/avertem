/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountGraphStore.hpp
 * Author: ubuntu
 *
 * Created on March 5, 2018, 7:30 AM
 */

#ifndef ACCOUNTGRAPHSTORE_HPP
#define ACCOUNTGRAPHSTORE_HPP

#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <librdf.h>
#include <redland.h>
#include <rdf_storage.h>
#include <rdf_model.h>

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace account_db {

class AccountGraphStore;
typedef std::shared_ptr<AccountGraphStore> AccountGraphStorePtr;
    
class AccountGraphStore {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    class StorageScopeLock;
    typedef std::shared_ptr<StorageScopeLock> StorageScopeLockPtr;

    class StorageLock {
    public:
        friend class StorageScopeLock;

        StorageLock();
        StorageLock(const StorageLock& orig) = delete;
        virtual ~StorageLock();

        StorageScopeLockPtr aquireReadLock();
        StorageScopeLockPtr aquireWriteLock();
    protected:
        void release(bool _readLock, bool _writeLock);
    public:
        std::mutex classMutex;
        std::condition_variable stateCondition;
        int readLock;
        int writeLock;
    };
    typedef std::shared_ptr<StorageLock> StorageLockPtr;

    class StorageScopeLock {
    public:
        StorageScopeLock(StorageLock* reference, bool readLock, bool writeLock);
        StorageScopeLock(const StorageScopeLock& orig) = delete;
        virtual ~StorageScopeLock();
    public:
        StorageLock* reference;
        bool readLock;
        bool writeLock;
    };
    
    friend class AccountGraphSession;
    AccountGraphStore(const std::string& dbName);
    AccountGraphStore(const AccountGraphStore& orig) = delete;
    virtual ~AccountGraphStore();
    
    std::string getDbName();
    
    static bool checkForDb(const std::string& dbName);
    
protected:
    librdf_world* getWorld();
    librdf_storage* getStorage();
    librdf_model* getModel();
    StorageLockPtr getStorageLock();
    
private:
    std::string dbName;
    librdf_world* world;
    librdf_storage* storage;
    librdf_model* model;
    StorageLockPtr storageLockPtr;
};


}
}


#endif /* ACCOUNTGRAPHSTORE_HPP */

