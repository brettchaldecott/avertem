
#include <sstream>
#include <botan/bcrypt.h>
#include <botan/hex.h>

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
    this->passwordPipeLinePtr = keto::crypto::PasswordPipeLinePtr(new keto::crypto::PasswordPipeLine());
    this->hash = this->passwordPipeLinePtr->generatePassword(password);
}

MemoryVaultManager::MemoryVaultWrapper::~MemoryVaultWrapper() {

}

keto::crypto::SecureVector MemoryVaultManager::MemoryVaultWrapper::getSessionId() {
    return this->sessionId;
}

MemoryVaultPtr MemoryVaultManager::MemoryVaultWrapper::getMemoryVault(const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (!(this->hash == this->passwordPipeLinePtr->generatePassword(password))) {
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

void MemoryVaultManager::fin() {
    singleton.reset();
}

MemoryVaultManagerPtr MemoryVaultManager::getInstance() {
    return singleton;
}


void MemoryVaultManager::createSession(
        const vectorOfSecureVectors& sessions) {
    std::lock_guard<std::mutex> guard(classMutex);
    this->vaults.clear();
    this->sessions.clear();
    for (keto::crypto::SecureVector vector : sessions) {
        //KETO_LOG_DEBUG << "[createSession] vectors : " << Botan::hex_encode(vector);
        this->sessions.insert(std::pair<keto::crypto::SecureVector,MemoryVaultWrapperPtr>(vector,MemoryVaultWrapperPtr()));
    }
}

void MemoryVaultManager::clearSession() {
    std::lock_guard<std::mutex> guard(classMutex);
    this->vaults.clear();
    this->sessions.clear();
}

MemoryVaultPtr MemoryVaultManager::createVault(const std::string& name,
                                  const keto::crypto::SecureVector& sessionId, const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    //KETO_LOG_DEBUG << "[createVault] vectors : " << Botan::hex_encode(sessionId);
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
    memoryVaultWrapperPtr = MemoryVaultWrapperPtr(new MemoryVaultWrapper(password,sessionId,memoryVaultPtr));
    this->sessions[sessionId] = memoryVaultWrapperPtr;
    this->vaults[name] = memoryVaultWrapperPtr;
    return memoryVaultPtr;
}

MemoryVaultPtr MemoryVaultManager::getVault(const std::string& name, const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (!this->vaults.count(name)) {
        std::stringstream ss;
        ss << "Unknown vault [" << name << "] cannot retrieve it.";
        BOOST_THROW_EXCEPTION(UnknownVaultException(ss.str()));
    }
    return this->vaults[name]->getMemoryVault(password);
}


}
}
