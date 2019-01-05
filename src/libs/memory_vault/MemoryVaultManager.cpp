

#include <botan/bcrypt.h>

#include "keto/memory_vault/MemoryVaultManager.hpp"
#include "keto/memory_vault/Exception.hpp"


namespace keto {
namespace memory_vault {

static MemoryVaultManagerPtr singleton = NULL;

std::string MemoryVaultManager::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVaultManager::MemoryVaultWrapper::MemoryVaultWrapper(const keto::crypto::SecureVector& password,
                                                           const keto::crypto::SecureVector& sessionId,
                                                           const MemoryVaultPtr& memoryVaultPtr)
                                                           : sessionId(sessionId), memoryVaultPtr(memoryVaultPtr) {
    std::string strPassword(password.begin(),password.end());
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());
    this->hash = Botan::generate_bcrypt(strPassword,*generator);
}

MemoryVaultManager::MemoryVaultWrapper::~MemoryVaultWrapper() {

}

keto::crypto::SecureVector MemoryVaultManager::MemoryVaultWrapper::getSessionId() {
    return this->sessionId;
}

MemoryVaultPtr MemoryVaultManager::MemoryVaultWrapper::getMemoryVault(const keto::crypto::SecureVector& password) {
    std::string strPassword(password.begin(),password.end());
    if (!Botan::check_bcrypt(strPassword,this->hash)) {
        BOOST_THROW_EXCEPTION(InvalidPasswordException());
    }
    return memoryVaultPtr;
}

MemoryVaultManager::MemoryVaultManager() {
    memoryVaultEncryptorPtr = MemoryVaultEncryptorPtr(new MemoryVaultEncryptor);
}

MemoryVaultManager::~MemoryVaultManager() {

}

MemoryVaultManagerPtr MemoryVaultManager::init() {
    return singleton = MemoryVaultManagerPtr(new MemoryVaultManager);
}

MemoryVaultManagerPtr MemoryVaultManager::fin() {
    singleton.reset();
}

MemoryVaultManagerPtr MemoryVaultManager::getInstance() {
    return singleton;
}


void MemoryVaultManager::createSession(
        const vectorOfSecureVectors& sessions) {
    this->vaults.clear();
    this->sessions.clear();
    for (keto::crypto::SecureVector vector : sessions) {
        this->sessions.insert(std::pair<keto::crypto::SecureVector,MemoryVaultWrapperPtr>(vector,MemoryVaultWrapperPtr()));
    }
}

void MemoryVaultManager::clearSession() {
    this->vaults.clear();
    this->sessions.clear();
}

MemoryVaultPtr MemoryVaultManager::createVault(const std::string& name,
                                  const keto::crypto::SecureVector& sessionId, const keto::crypto::SecureVector& password) {
    if (!this->sessions.count(sessionId)) {
        BOOST_THROW_EXCEPTION(InvalidSesssionException());
    }
    MemoryVaultWrapperPtr memoryVaultWrapperPtr = this->sessions[sessionId];
    if (memoryVaultWrapperPtr) {
        BOOST_THROW_EXCEPTION(DuplicateVaultException());
    }
    if (this->vaults.count(name)) {
        BOOST_THROW_EXCEPTION(DuplicateVaultException());
    }
    MemoryVaultPtr memoryVaultPtr(new MemoryVault(sessionId,this->memoryVaultEncryptorPtr));
    memoryVaultWrapperPtr = MemoryVaultWrapperPtr(new MemoryVaultWrapper(sessionId,password,memoryVaultPtr));
    this->sessions[sessionId] = memoryVaultWrapperPtr;
    this->vaults[name] = memoryVaultWrapperPtr;
    return memoryVaultPtr;
}

MemoryVaultPtr MemoryVaultManager::getVault(const std::string& name, const keto::crypto::SecureVector& password) {
    if (!this->vaults.count(name)) {
        BOOST_THROW_EXCEPTION(UnknownVaultException());
    }
    return this->vaults[name]->getMemoryVault(password);
}


}
}