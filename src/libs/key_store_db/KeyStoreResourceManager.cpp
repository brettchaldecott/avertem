//
// Created by Brett Chaldecott on 2018/11/25.
//

#include "include/keto/key_store_db/KeyStoreResourceManager.hpp"

#include "keto/server_common/TransactionHelper.hpp"

namespace keto {
namespace key_store_db {


thread_local KeyStoreResourcePtr KeyStoreResourceManager::keyStoreResourcePtr;

std::string KeyStoreResourceManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreResourceManager::KeyStoreResourceManager(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr) :
        dbManagerPtr(dbManagerPtr) {
}


KeyStoreResourceManager::~KeyStoreResourceManager() {
}


void KeyStoreResourceManager::commit() {
    if (keyStoreResourcePtr) {
        keyStoreResourcePtr->commit();
        keyStoreResourcePtr.reset();
    }
}


void KeyStoreResourceManager::rollback() {
    if (keyStoreResourcePtr) {
        keyStoreResourcePtr->rollback();
        keyStoreResourcePtr.reset();
    }
}


KeyStoreResourcePtr KeyStoreResourceManager::getResource() {
    if (!keyStoreResourcePtr) {
        keyStoreResourcePtr = KeyStoreResourcePtr(new KeyStoreResource(dbManagerPtr));
        keto::server_common::enlistResource(*this);
    }
    return keyStoreResourcePtr;
}


}
}
