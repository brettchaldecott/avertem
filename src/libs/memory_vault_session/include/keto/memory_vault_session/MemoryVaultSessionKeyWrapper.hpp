//
// Created by Brett Chaldecott on 2019/01/15.
//

#ifndef KETO_MEMORYVAULTSESSIONKEYWRAPPER_HPP
#define KETO_MEMORYVAULTSESSIONKEYWRAPPER_HPP

#include <memory>
#include <string>
#include <vector>

#include <botan/hex.h>
#include <botan/pkcs8.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/x509_key.h>



#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/crypto/PasswordPipeLine.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"


#include "keto/memory_vault_session/MemoryVaultSessionEntry.hpp"


namespace keto {
namespace memory_vault_session {

class MemoryVaultSessionKeyWrapper;
typedef std::shared_ptr<MemoryVaultSessionKeyWrapper> MemoryVaultSessionKeyWrapperPtr;

class MemoryVaultSessionKeyWrapper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVaultSessionKeyWrapper(const std::shared_ptr<Botan::Private_Key>& privateKey);
    MemoryVaultSessionKeyWrapper(const std::shared_ptr<Botan::Public_Key>& privateKey);
    MemoryVaultSessionKeyWrapper(const MemoryVaultSessionKeyWrapper& orig) = delete;
    virtual ~MemoryVaultSessionKeyWrapper();


    std::shared_ptr<Botan::Private_Key> getPrivateKey();
    MemoryVaultSessionKeyWrapper& operator = (const std::shared_ptr<Botan::Private_Key>& key);
    std::shared_ptr<Botan::Public_Key> getPublicKey();
    MemoryVaultSessionKeyWrapper& operator = (const std::shared_ptr<Botan::Public_Key>& key);

    operator std::shared_ptr<Botan::Private_Key>();
    operator std::shared_ptr<Botan::Public_Key>();

private:
    keto::crypto::SecureVector hashId;
    MemoryVaultSessionEntryPtr memoryVaultSessionEntryPtr;
};

}
}


#endif //KETO_MEMORYVAULTSESSIONKEYWRAPPER_HPP
