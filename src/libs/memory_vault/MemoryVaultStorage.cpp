//
// Created by Brett Chaldecott on 2018/12/30.
//

#include <botan/hex.h>

#include "keto/memory_vault/MemoryVaultStorage.hpp"
#include "keto/memory_vault/Exception.hpp"


namespace keto {
namespace memory_vault {

std::string MemoryVaultStorage::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


MemoryVaultStorage::MemoryVaultStorage() {

}

MemoryVaultStorage::~MemoryVaultStorage() {

}


void MemoryVaultStorage::setEntry(const keto::crypto::SecureVector& id,
              const byteVector bytes) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    //std::cout << "Set the entry : " << Botan::hex_encode(id) << std::endl;
    this->store[id] = bytes;
}

byteVector MemoryVaultStorage::getEntry(const keto::crypto::SecureVector& id) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    //std::cout << "Get the entry : " << Botan::hex_encode(id) << std::endl;
    if (!this->store.count(id)) {
        BOOST_THROW_EXCEPTION(VaultEntryNotFoundException());
    }
    return this->store[id];
}

void MemoryVaultStorage::removeEntry(const keto::crypto::SecureVector& id) {
    this->store.erase(id);
}


}
}