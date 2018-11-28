//
// Created by Brett Chaldecott on 2018/11/27.
//

#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"

namespace keto {
namespace keystore {

static KeyStoreStorageManagerPtr singleton;

std::string KeyStoreStorageManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreStorageManager::KeyStoreStorageManager() {

}


KeyStoreStorageManager::~KeyStoreStorageManager() {

}


KeyStoreStorageManagerPtr KeyStoreStorageManager::init() {
    keto::key_store_db::KeyStoreDB::init();
    return singleton = KeyStoreStorageManagerPtr(new KeyStoreStorageManager());
}

void KeyStoreStorageManager::fin() {
    singleton.reset();
    keto::key_store_db::KeyStoreDB::fin();
}

KeyStoreStorageManagerPtr KeyStoreStorageManager::getInstance() {
    return singleton;
}



}
}


