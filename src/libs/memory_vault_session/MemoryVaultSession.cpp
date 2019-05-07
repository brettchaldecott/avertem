//
// Created by Brett Chaldecott on 2019/01/14.
//

#include "MemoryVault.pb.h"

#include "keto/event/Event.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/memory_vault_session/MemoryVaultSession.hpp"
#include "keto/memory_vault_session/Exception.hpp"
#include "keto/memory_vault_session/PasswordCache.hpp"
#include "keto/crypto/HashGenerator.hpp"

namespace keto {
namespace memory_vault_session {

static MemoryVaultSessionPtr singleton;


PasswordCachePtr passwordCachePtr;


std::string MemoryVaultSession::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVaultSession::MemoryVaultSession(
        const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator,
        const std::string& vaultName) :
        consensusHashGenerator(consensusHashGenerator), vaultName(vaultName) {
    passwordPipeLinePtr = keto::crypto::PasswordPipeLinePtr(new keto::crypto::PasswordPipeLine());
}

MemoryVaultSession::~MemoryVaultSession() {
    passwordCachePtr.reset();
}

MemoryVaultSessionPtr MemoryVaultSession::init(
        const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator,
        const std::string& vaultName) {
    return singleton = MemoryVaultSessionPtr(new MemoryVaultSession(consensusHashGenerator,vaultName));
}

void MemoryVaultSession::fin() {
    singleton.reset();
}

MemoryVaultSessionPtr MemoryVaultSession::getInstance() {
    return singleton;
}

void MemoryVaultSession::initSession() {
    keto::proto::MemoryVaultCreate request;
    request.set_vault(this->vaultName);
    request.set_session(keto::crypto::SecureVectorUtils().copySecureToString(
            this->consensusHashGenerator->getCurrentSoftwareHash()));
    request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(generatePassword()));

    keto::proto::MemoryVaultCreate response =
            keto::server_common::fromEvent<keto::proto::MemoryVaultCreate>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MemoryVaultCreate>(
                                    keto::server_common::Events::MEMORY_VAULT::CREATE_VAULT,request)));

}

void MemoryVaultSession::clearSession() {
    this->sessionEntries.clear();
}

// entry management methods
MemoryVaultSessionEntryPtr MemoryVaultSession::createEntry(const std::string& name) {
    if (this->sessionEntries.count(name)) {
        //BOOST_THROW_EXCEPTION(DuplicateMemoryVaultSessionException());
        // return the session entry
        return this->sessionEntries[name];
    }

    MemoryVaultSessionEntryPtr memoryVaultSessionEntryPtr(new MemoryVaultSessionEntry(
            this, keto::crypto::HashGenerator().generateHash(name)));
    this->sessionEntries[name] = memoryVaultSessionEntryPtr;
    return memoryVaultSessionEntryPtr;
}

MemoryVaultSessionEntryPtr MemoryVaultSession::createEntry(const std::string& name, const keto::crypto::SecureVector& value) {
    if (this->sessionEntries.count(name)) {
        //BOOST_THROW_EXCEPTION(DuplicateMemoryVaultSessionException());
        // return the session entry
        return this->sessionEntries[name];
    }

    MemoryVaultSessionEntryPtr memoryVaultSessionEntryPtr(new MemoryVaultSessionEntry(this,
            keto::crypto::HashGenerator().generateHash(name), value));
    this->sessionEntries[name] = memoryVaultSessionEntryPtr;
    return memoryVaultSessionEntryPtr;
}

MemoryVaultSessionEntryPtr MemoryVaultSession::getEntry(const std::string& name) {
    if (!this->sessionEntries.count(name)) {
        BOOST_THROW_EXCEPTION(UnknownMemoryVaultSessionException());
    }
    return this->sessionEntries[name];
}

void MemoryVaultSession::removeEntry(const std::string& name) {
    if (!this->sessionEntries.count(name)) {
        //BOOST_THROW_EXCEPTION(UnknownMemoryVaultSessionException());
        // ignore if the entry is not found
        return;
    }
    this->sessionEntries.erase(name);
}

keto::crypto::SecureVector MemoryVaultSession::generatePassword() {
    if (!passwordCachePtr || passwordCachePtr->isExpired()) {
        keto::crypto::SecureVector seed = keto::crypto::SecureVectorUtils().copyStringToSecure(this->vaultName);
        keto::crypto::SecureVector key = this->consensusHashGenerator->getCurrentSoftwareHash();
        seed.insert(seed.begin(), key.begin(), key.begin());
        passwordCachePtr = PasswordCachePtr(new PasswordCache(this->consensusHashGenerator->generateSessionHash(seed)));
    }
    return this->passwordPipeLinePtr->generatePassword(passwordCachePtr->getSeedHash());
}

std::string MemoryVaultSession::getVaultName() {
    return this->vaultName;
}

}
}
