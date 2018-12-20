//
// Created by Brett Chaldecott on 2018/11/24.
//

#ifndef KETO_KEYSTOREDB_HPP
#define KETO_KEYSTOREDB_HPP

#include <memory>
#include <string>
#include <vector>

#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/key_store_db/KeyStoreResourceManager.hpp"

namespace keto {
namespace key_store_db {


class KeyStoreDB;
typedef std::shared_ptr<KeyStoreDB> KeyStoreDBPtr;
typedef std::shared_ptr<Botan::Private_Key> PrivateKeyPtr;
typedef std::vector<PrivateKeyPtr> OnionKeys;

class KeyStoreDB {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();


    KeyStoreDB(const KeyStoreDB& origin) = delete;
    virtual ~KeyStoreDB();

    static KeyStoreDBPtr init(const PrivateKeyPtr& baseKey);
    static void fin();
    static KeyStoreDBPtr getInstance();

    void setValue(const keto::crypto::SecureVector& key, const keto::crypto::SecureVector& value, const OnionKeys& onionKeys);
    void setValue(const std::string& key, const keto::crypto::SecureVector& value, const OnionKeys& onionKeys);
    void setValue(const std::string& key, const std::string& value, const OnionKeys& onionKeys);
    bool getValue(const keto::crypto::SecureVector& key, const OnionKeys& onionKeys, keto::crypto::SecureVector& bytes);
    bool getValue(const std::string& key, const OnionKeys& onionKeys, keto::crypto::SecureVector& bytes);
    bool getValue(const std::string& key, const OnionKeys& onionKeys, std::string& value);


private:
    PrivateKeyPtr baseKey;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    KeyStoreResourceManagerPtr keyStoreResourceManagerPtr;

    KeyStoreDB(const PrivateKeyPtr& baseKey);



};


}
}


#endif //KETO_KEYSTOREDB_HPP
