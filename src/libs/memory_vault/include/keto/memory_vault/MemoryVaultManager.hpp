


#ifndef KETO_MEMORYVAULTMANAGER_HPP
#define KETO_MEMORYVAULTMANAGER_HPP


#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/crypto/PasswordPipeLine.hpp"

#include "keto/memory_vault/MemoryVaultEncryptor.hpp"
#include "keto/memory_vault/MemoryVaultStorage.hpp"
#include "keto/memory_vault/MemoryVault.hpp"


namespace keto {
namespace memory_vault {

class MemoryVaultManager;
typedef std::shared_ptr<MemoryVaultManager> MemoryVaultManagerPtr;
typedef std::vector<keto::crypto::SecureVector> vectorOfSecureVectors;
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
        bool validPassword(const keto::crypto::SecureVector& password);

    private:
        std::mutex classMutex;
        keto::crypto::SecureVector sessionId;
        MemoryVaultPtr memoryVaultPtr;
        keto::crypto::PasswordPipeLinePtr passwordPipeLinePtr;
        keto::crypto::SecureVector hash;

    };
    typedef std::shared_ptr<MemoryVaultWrapper> MemoryVaultWrapperPtr;

    class MemoryVaultSlot {
    public:
        MemoryVaultSlot(const uint8_t& slot,const MemoryVaultEncryptorPtr& memoryVaultEncryptorPtr, const vectorOfSecureVectors& sessions);
        MemoryVaultSlot(const MemoryVaultSlot& orig) = delete;
        virtual ~MemoryVaultSlot();

        uint8_t getSlot();
        MemoryVaultPtr createVault(const std::string& name,
                                   const keto::crypto::SecureVector& sessionId, const keto::crypto::SecureVector& password);
        MemoryVaultPtr getVault(const std::string& name, const keto::crypto::SecureVector& password);
        bool isVault(const std::string& name, const keto::crypto::SecureVector& password);

    private:
        uint8_t slot;
        MemoryVaultEncryptorPtr memoryVaultEncryptorPtr;
        std::mutex classMutex;
        std::map<keto::crypto::SecureVector,MemoryVaultWrapperPtr> sessions;
        std::map<std::string,MemoryVaultWrapperPtr> vaults;
    };
    typedef std::shared_ptr<MemoryVaultSlot> MemoryVaultSlotPtr;

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
    MemoryVaultPtr getVault(const uint8_t& slot, const std::string& name, const keto::crypto::SecureVector& password);



private:
    std::mutex classMutex;
    uint8_t currentSlot;
    MemoryVaultEncryptorPtr memoryVaultEncryptorPtr;
    std::deque<MemoryVaultSlotPtr> slots;
    std::map<uint8_t,MemoryVaultSlotPtr> slotIndex;

    uint8_t nextSlot();
};


}
}


#endif


