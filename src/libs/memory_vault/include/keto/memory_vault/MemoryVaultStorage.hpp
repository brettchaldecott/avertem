//
// Created by Brett Chaldecott on 2018/12/30.
//

#ifndef KETO_MEMORYVAULTSTORAGE_HPP
#define KETO_MEMORYVAULTSTORAGE_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>


#include "keto/obfuscate/MetaString.hpp"
#include "keto/crypto/Containers.hpp"


namespace keto {
namespace memory_vault {

class MemoryVaultStorage;
typedef std::shared_ptr<MemoryVaultStorage> MemoryVaultStoragePtr;
typedef std::vector<uint8_t> byteVector;

class MemoryVaultStorage {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:");
    };
    static std::string getSourceVersion();

    class MemoryVaultStorageEntry {
    public:
        MemoryVaultStorageEntry(const byteVector& bytes);
        MemoryVaultStorageEntry(const MemoryVaultStorageEntry& memoryVaultStorageEntry) = delete;
        virtual ~MemoryVaultStorageEntry();

        operator byteVector();
        byteVector getBytes();
        int incrementRefCount();
        int decrementRefCount();

    private:
        std::mutex classMutex;
        int refCount;
        byteVector bytes;
    };
    typedef std::shared_ptr<MemoryVaultStorageEntry> MemoryVaultStorageEntryPtr;

    MemoryVaultStorage();
    MemoryVaultStorage(const MemoryVaultStorage& orig) = delete;
    virtual ~MemoryVaultStorage();


    void setEntry(const keto::crypto::SecureVector& id,
            const byteVector bytes);

    byteVector getEntry(const keto::crypto::SecureVector& id);

    void removeEntry(const keto::crypto::SecureVector& id);

private:
    std::mutex classMutex;
    std::map<keto::crypto::SecureVector,MemoryVaultStorageEntryPtr> store;

};


}
}



#endif //KETO_MEMORYVAULTSTORAGE_HPP
