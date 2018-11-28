//
// Created by Brett Chaldecott on 2018/11/25.
//

#include "keto/key_store_db/KeyStoreResource.hpp"
#include "keto/key_store_db/Constants.hpp"

namespace keto {
namespace key_store_db {


std::string KeyStoreResource::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


KeyStoreResource::KeyStoreResource(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr) :
        dbManagerPtr(dbManagerPtr), transaction(NULL) {

}


KeyStoreResource::~KeyStoreResource() {
    if (transaction) {
        transaction->Rollback();
        delete transaction;
        transaction = NULL;
    }
}

void KeyStoreResource::commit() {
    if (transaction) {
        transaction->Commit();
        delete transaction;
        transaction = NULL;
    }
}

void KeyStoreResource::rollback() {
    if (transaction) {
        transaction->Rollback();
        delete transaction;
        transaction = NULL;
    }
}

rocksdb::Transaction* KeyStoreResource::getTransaction() {
    if (!transaction) {
        keto::rocks_db::DBConnectorPtr dbConnectorPtr =
                dbManagerPtr->getConnection(Constants::KEY_STORE);
        rocksdb::WriteOptions write_options;
        this->transaction = dbConnectorPtr->getDB()->BeginTransaction(
                write_options);

    }
    return this->transaction;
}



}
}