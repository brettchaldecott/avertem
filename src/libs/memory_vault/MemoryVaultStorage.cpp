//
// Created by Brett Chaldecott on 2018/12/30.
//

#include <sstream>

#include <botan/hex.h>

#include "keto/memory_vault/MemoryVaultStorage.hpp"
#include "keto/memory_vault/Exception.hpp"


namespace keto {
namespace memory_vault {

std::string MemoryVaultStorage::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

MemoryVaultStorage::MemoryVaultStorageEntry::MemoryVaultStorageEntry(const byteVector& bytes) :
    refCount(1), bytes(bytes) {

}

MemoryVaultStorage::MemoryVaultStorageEntry::~MemoryVaultStorageEntry() {

}

MemoryVaultStorage::MemoryVaultStorageEntry::operator byteVector() {
    return bytes;
}

byteVector MemoryVaultStorage::MemoryVaultStorageEntry::getBytes() {
    return bytes;
}

int MemoryVaultStorage::MemoryVaultStorageEntry::incrementRefCount() {
    return refCount++;
}

int MemoryVaultStorage::MemoryVaultStorageEntry::decrementRefCount() {
    return refCount--;
}

MemoryVaultStorage::MemoryVaultStorage() {

}

MemoryVaultStorage::~MemoryVaultStorage() {

}


void MemoryVaultStorage::setEntry(const keto::crypto::SecureVector& id,
              const byteVector bytes) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->store.count(id)) {
        std::cout << "[MemoryVaultStorage][setEntry] Increment the reference count for [" << Botan::hex_encode(id) << "]" << std::endl;
        this->store[id]->incrementRefCount();
    } else {
        std::cout << "[MemoryVaultStorage][setEntry] Add the entry [" << Botan::hex_encode(id) << "]" << std::endl;
        this->store[id] = MemoryVaultStorageEntryPtr(new MemoryVaultStorageEntry(bytes));
    }

}

byteVector MemoryVaultStorage::getEntry(const keto::crypto::SecureVector& id) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    //std::cout << "Get the entry : " << Botan::hex_encode(id) << std::endl;
    if (!this->store.count(id)) {
        std::stringstream ss;
        ss << "[VaultEntryNotFoundException]The entry [" << Botan::hex_encode(id) << "][" << this->store.size() << "]";
        //for(std::map<keto::crypto::SecureVector,MemoryVaultStorageEntryPtr>::iterator iter = this->store.begin(); iter != this->store.end(); ++iter) {
        //    ss << "[" << Botan::hex_encode(iter->first) << "]";
        //}
        BOOST_THROW_EXCEPTION(VaultEntryNotFoundException(ss.str()));
    }
    return this->store[id]->getBytes();
}

void MemoryVaultStorage::removeEntry(const keto::crypto::SecureVector& id) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    std::cout << "[MemoryVaultStorage][removeEntry] Remove the entry [" << Botan::hex_encode(id) << "]" << std::endl;
    if (!this->store.count(id)) {
        return;
    }
    if (this->store[id]->decrementRefCount() <= 0) {
        this->store.erase(id);
    }
}


}
}