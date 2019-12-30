
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


bool MemoryVaultManager::MemoryVaultWrapper::validPassword(const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    return this->hash == this->passwordPipeLinePtr->generatePassword(password);
}

MemoryVaultManager::MemoryVaultSlot::MemoryVaultSlot(const uint8_t& slot, const MemoryVaultEncryptorPtr& memoryVaultEncryptorPtr, const vectorOfSecureVectors& sessions) :
    slot(slot),  memoryVaultEncryptorPtr(memoryVaultEncryptorPtr) {
    for (keto::crypto::SecureVector vector : sessions) {
        //KETO_LOG_DEBUG << "[createSession] vectors : " << Botan::hex_encode(vector);
        this->sessions.insert(std::pair<keto::crypto::SecureVector,MemoryVaultWrapperPtr>(vector,MemoryVaultWrapperPtr()));
    }
}

MemoryVaultManager::MemoryVaultSlot::~MemoryVaultSlot() {
    KETO_LOG_ERROR << "The slot destructor : " << (int)slot;
}


uint8_t MemoryVaultManager::MemoryVaultSlot::getSlot() {
    return slot;
}

MemoryVaultPtr MemoryVaultManager::MemoryVaultSlot::createVault(const std::string& name,
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
    MemoryVaultPtr memoryVaultPtr(new MemoryVault(this->getSlot(),sessionId,this->memoryVaultEncryptorPtr));
    memoryVaultWrapperPtr = MemoryVaultWrapperPtr(new MemoryVaultWrapper(password,sessionId,memoryVaultPtr));
    this->sessions[sessionId] = memoryVaultWrapperPtr;
    this->vaults[name] = memoryVaultWrapperPtr;
    return memoryVaultPtr;
}

MemoryVaultPtr MemoryVaultManager::MemoryVaultSlot::getVault(const std::string& name, const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (!this->vaults.count(name)) {
        std::stringstream ss;
        ss << "Unknown vault [" << name << "] cannot retrieve it.";
        BOOST_THROW_EXCEPTION(UnknownVaultException(ss.str()));
    }
    return this->vaults[name]->getMemoryVault(password);
}

bool MemoryVaultManager::MemoryVaultSlot::isVault(const std::string& name, const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (!this->vaults.count(name)) {
        return false;
    }
    return this->vaults[name]->validPassword(password);
}

MemoryVaultManager::MemoryVaultManager() : currentSlot(0) {
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
    uint8_t slotId = nextSlot();
    MemoryVaultSlotPtr memoryVaultSlotPtr(new MemoryVaultSlot(slotId,this->memoryVaultEncryptorPtr,sessions));
    this->slots.push_front(memoryVaultSlotPtr);
    this->slotIndex[slotId] = memoryVaultSlotPtr;
}

void MemoryVaultManager::clearSession() {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->slots.size() <= 2) {
        return;
    }
    this->slotIndex.erase(this->slots.back()->getSlot());
    this->slots.pop_back();
}

MemoryVaultPtr MemoryVaultManager::createVault(const std::string& name,
                                  const keto::crypto::SecureVector& sessionId, const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    return this->slots.front()->createVault(name,sessionId,password);
}

MemoryVaultPtr MemoryVaultManager::getVault(const uint8_t& slot, const std::string& name, const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    if (this->slotIndex.count(slot)) {
        return this->slotIndex[slot]->getVault(name,password);
    }

    std::stringstream ss;
    ss << "Unknown vault [" << slot << "][" << name << "] cannot retrieve it.";
    BOOST_THROW_EXCEPTION(UnknownVaultException(ss.str()));
}

uint8_t MemoryVaultManager::nextSlot() {
    ++currentSlot;
    if (currentSlot >= 255) {
        currentSlot=1;
    }
    return currentSlot;
}

}
}
