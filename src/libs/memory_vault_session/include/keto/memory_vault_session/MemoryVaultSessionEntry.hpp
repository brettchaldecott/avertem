//
// Created by Brett Chaldecott on 2019/01/14.
//

#ifndef KETO_MEMORYVAULTSESSIONENTRY_HPP
#define KETO_MEMORYVAULTSESSIONENTRY_HPP

#include <memory>
#include <string>
#include <vector>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/crypto/PasswordPipeLine.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"


namespace keto {
namespace memory_vault_session {

class MemoryVaultSessionEntry;
typedef std::shared_ptr<MemoryVaultSessionEntry> MemoryVaultSessionEntryPtr;

class MemoryVaultSession;

class MemoryVaultSessionEntry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    MemoryVaultSessionEntry(MemoryVaultSession* memoryVaultSession, const keto::crypto::SecureVector& entryId);
    MemoryVaultSessionEntry(MemoryVaultSession* memoryVaultSession, const keto::crypto::SecureVector& entryId, const keto::crypto::SecureVector& value);
    MemoryVaultSessionEntry(const MemoryVaultSessionEntry& orig) = delete;
    virtual ~MemoryVaultSessionEntry();

    keto::crypto::SecureVector getValue();
    void setValue(const keto::crypto::SecureVector& value);

    keto::crypto::SecureVector getEntryId();

private:
    bool valueIsSet;
    MemoryVaultSession* memoryVaultSession;
    keto::crypto::SecureVector entryId;



};

}
}

#endif //KETO_MEMORYVAULTSESSIONENTRY_HPP
