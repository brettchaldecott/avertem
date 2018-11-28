//
// Created by Brett Chaldecott on 2018/11/24.
//

#ifndef KETO_KEYSTOREDB_HPP
#define KETO_KEYSTOREDB_HPP

#include <memory>
#include <string>
#include <vector>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/key_store_db/KeyStoreResourceManager.hpp"

namespace keto {
namespace key_store_db {


class KeyStoreDB;
typedef std::shared_ptr<KeyStoreDB> KeyStoreDBPtr;
typedef std::vector<keto::crypto::SecureVector> OnionKeys;

class KeyStoreDB {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();


    KeyStoreDB(const KeyStoreDB& origin) = delete;
    virtual ~KeyStoreDB();

    static KeyStoreDBPtr init();
    static void fin();
    static KeyStoreDBPtr getInstance();

    void setValue(const keto::crypto::SecureVector& key, const keto::crypto::SecureVector& value, const OnionKeys& onionKeys);
    keto::crypto::SecureVector getValue(const keto::crypto::SecureVector& key, const OnionKeys& onionKeys);



private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    KeyStoreResourceManagerPtr keyStoreResourceManagerPtr;

    KeyStoreDB();



};


}
}


#endif //KETO_KEYSTOREDB_HPP
