//
// Created by Brett Chaldecott on 2018/11/27.
//

#ifndef KETO_KEYSTORESTORAGEMANAGER_HPP
#define KETO_KEYSTORESTORAGEMANAGER_HPP

#include <string>
#include <memory>


#include "keto/key_store_db/KeyStoreDB.hpp"

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace keystore {

class KeyStoreStorageManager;
typedef std::shared_ptr<KeyStoreStorageManager> KeyStoreStorageManagerPtr;

class KeyStoreStorageManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    KeyStoreStorageManager();
    KeyStoreStorageManager(const KeyStoreStorageManager& orig) = delete;
    virtual ~KeyStoreStorageManager();


    static KeyStoreStorageManagerPtr init();
    static void fin();
    static KeyStoreStorageManagerPtr getInstance();




private:

};


}
}


#endif //KETO_KEYSTORESTORAGEMANAGER_HPP
