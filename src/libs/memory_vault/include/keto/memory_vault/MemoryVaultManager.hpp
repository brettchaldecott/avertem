


#ifndef KETO_MEMORYVAULTMANAGER_HPP
#define KETO_MEMORYVAULTMANAGER_HPP


#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"

#include "keto/memory_vault/MemoryVaultEncryptor.hpp"
#include "keto/memory_vault/MemoryVaultStorage.hpp"
#include "keto/memory_vault/MemoryVault.hpp"


namespace keto {
namespace memory_vault {

class MemoryVaultManager;
typedef std::shared_ptr<MemoryVaultManager> MemoryVaultManagerPtr;
typedef std::vector<keto::crypto::SecureVector> vectorOfSecureVectors;
typedef std::set<keto::crypto::SecureVector> setOfSecureVectors;
typedef std::vector<uint8_t> bytes;

class MemoryVaultManager {
public:
    class MemoryVaultWrapper {
    public:
        MemoryVaultWrapper(const keto::crypto::SecureVector& password, const keto::crypto::SecureVector& sessionId,
                const MemoryVaultPtr& memoryVaultPtr);
        MemoryVaultWrapper(const MemoryVaultWrapper& orig) = delete;
        virtual ~MemoryVaultWrapper();

        keto::crypto::SecureVector getSessionId();
        MemoryVaultPtr getMemoryVault(const keto::crypto::SecureVector& password);
    private:
        keto::crypto::SecureVector sessionId;
        MemoryVaultPtr memoryVaultPtr;
        std::string hash;

    };
    typedef std::shared_ptr<MemoryVaultWrapper> MemoryVaultWrapperPtr;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVaultManager();
    MemoryVaultManager(const MemoryVaultManager& original) = delete;
    virtual ~MemoryVaultManager();


    static MemoryVaultManagerPtr init();
    static void fin();
    static MemoryVaultManagerPtr getInstance();


    void createSession(
            const vectorOfSecureVectors& sessions);
    void clearSession();

    MemoryVaultPtr createVault(const std::string& name,
                const keto::crypto::SecureVector& sessionId, const keto::crypto::SecureVector& password);
    MemoryVaultPtr getVault(const std::string& name, const keto::crypto::SecureVector& password);



private:
    std::map<keto::crypto::SecureVector,MemoryVaultWrapperPtr> sessions;
    MemoryVaultEncryptorPtr memoryVaultEncryptorPtr;
    std::map<std::string,MemoryVaultWrapperPtr> vaults;


};


}
}


#endif


