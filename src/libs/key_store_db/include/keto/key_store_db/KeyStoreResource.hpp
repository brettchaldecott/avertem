//
// Created by Brett Chaldecott on 2018/11/25.
//

#ifndef KETO_KEYSTORERESOURCE_HPP
#define KETO_KEYSTORERESOURCE_HPP

#include <string>
#include <memory>


#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

#include "keto/transaction/Resource.hpp"
#include "keto/rocks_db/DBManager.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_store_db {

class KeyStoreResource;
typedef std::shared_ptr<KeyStoreResource> KeyStoreResourcePtr;

class KeyStoreResource {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    KeyStoreResource(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr);
    KeyStoreResource(const KeyStoreResource& orig) = delete;
    virtual ~KeyStoreResource();

    void commit();
    void rollback();

    rocksdb::Transaction* getTransaction();


private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    rocksdb::Transaction* transaction;
};


}
}


#endif //KETO_KEYSTORERESOURCE_HPP
