//
// Created by Brett Chaldecott on 2019/01/14.
//

#ifndef KETO_MEMORYVAULTSESSION_HPP
#define KETO_MEMORYVAULTSESSION_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/crypto/PasswordPipeLine.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/memory_vault_session/MemoryVaultSessionEntry.hpp"
#include "keto/memory_vault_session/PasswordCache.hpp"

namespace keto {
namespace memory_vault_session {

class MemoryVaultSession;
typedef std::shared_ptr<MemoryVaultSession> MemoryVaultSessionPtr;

class MemoryVaultSession {
public:

    friend class MemoryVaultSessionEntry;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVaultSession(
            const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator,
            const std::string& vaultName);
    MemoryVaultSession(const MemoryVaultSession& orig) = delete;
    virtual ~MemoryVaultSession();

    static MemoryVaultSessionPtr init(
            const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator,
            const std::string& vaultName);
    static void fin();
    static MemoryVaultSessionPtr getInstance();

    void initSession();
    void clearSession();

    // entry management methods
    MemoryVaultSessionEntryPtr createEntry(const std::string& name);
    MemoryVaultSessionEntryPtr createEntry(const std::string& name, const keto::crypto::SecureVector& value);
    MemoryVaultSessionEntryPtr getEntry(const std::string& name);
    void removeEntry(const std::string& name);



protected:
    keto::memory_vault_session::PasswordCachePtr generatePassword();
    keto::crypto::SecureVector processPassword(const keto::memory_vault_session::PasswordCachePtr _passwordCachePtr);
    std::string getVaultName();
    uint8_t getSlot();

private:
    std::recursive_mutex classMutex;
    keto::memory_vault_session::PasswordCachePtr passwordCachePtr;
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator;
    std::string vaultName;
    keto::crypto::PasswordPipeLinePtr passwordPipeLinePtr;
    uint8_t slot;
    std::map<std::string,MemoryVaultSessionEntryPtr> sessionEntries;

    keto::crypto::SecureVector processPassword();
};


}
}


#endif //KETO_MEMORYVAULTSESSION_HPP
