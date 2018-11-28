//
// Created by Brett Chaldecott on 2018/11/25.
//

#ifndef KETO_KEYSTORERESOURCEMANAGER_HPP
#define KETO_KEYSTORERESOURCEMANAGER_HPP

#include <string>
#include <memory>

#include "keto/transaction/Resource.hpp"
#include "keto/rocks_db/DBManager.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/key_store_db/KeyStoreResource.hpp"

namespace keto {
namespace key_store_db {


class KeyStoreResourceManager;
typedef std::shared_ptr<KeyStoreResourceManager> KeyStoreResourceManagerPtr;

class KeyStoreResourceManager  : keto::transaction::Resource {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    KeyStoreResourceManager(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr);
    KeyStoreResourceManager(const KeyStoreResourceManager& orig) = delete;
    virtual ~KeyStoreResourceManager();

    virtual void commit();
    virtual void rollback();


    KeyStoreResourcePtr getResource();

private:
    static thread_local KeyStoreResourcePtr keyStoreResourcePtr;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;

};


}
}



#endif //KETO_KEYSTORERESOURCEMANAGER_HPP
