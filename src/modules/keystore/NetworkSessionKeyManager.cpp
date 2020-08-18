//
// Created by Brett Chaldecott on 2019/01/23.
//

#include <algorithm>

#include <keto/crypto/HashGenerator.hpp>
#include <keto/crypto/SecureVectorUtils.hpp>
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/server_common/EventUtils.hpp"


#include "keto/keystore/Exception.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/MasterKeyManager.hpp"
#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"

#include "keto/software_consensus/ConsensusStateManager.hpp"



namespace keto {
namespace keystore {

static NetworkSessionKeyManagerPtr singleton;

std::string NetworkSessionKeyManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


NetworkSessionKeyManager::NetworkSessionSlot::NetworkSessionSlot(
        uint8_t slot, std::vector<std::vector<uint8_t>> hashIndex, const SessionMap& sessionKeys) :
        slot(slot), hashIndex(hashIndex), sessionKeys(sessionKeys) {

}
NetworkSessionKeyManager::NetworkSessionSlot::~NetworkSessionSlot() {
    this->hashIndex.clear();
    this->sessionKeys.clear();
}

int NetworkSessionKeyManager::NetworkSessionSlot::getNumberOfKeys() {
    return this->hashIndex.size();
}

keto::proto::NetworkKeysWrapper NetworkSessionKeyManager::NetworkSessionSlot::getSession() {
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper;
    for (std::map<std::vector<uint8_t>,keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr>::iterator session =
            this->sessionKeys.begin(); session != this->sessionKeys.end(); ++session) {
        keto::rpc_protocol::NetworkKeyHelper networkKeyHelper;
        networkKeyHelper.setHash(session->first);
        networkKeyHelper.setKeyBytes(Botan::PKCS8::BER_encode(*session->second->getPrivateKey()));
        networkKeysHelper.addNetworkKey(networkKeyHelper);
    }
    networkKeysHelper.setSlot(this->slot);
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper;
    networkKeysWrapperHelper.setBytes(
            NetworkSessionKeyManager::getInstance()->consensusHashGenerator->encrypt((keto::crypto::SecureVector)networkKeysHelper));
    return networkKeysWrapperHelper;
}

keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr NetworkSessionKeyManager::NetworkSessionSlot::getKey(int index) {
    if (index >= this->hashIndex.size()) {
        std::stringstream ss;
        ss << "Index out of bounds [" << index << "][" << this->hashIndex.size() << "]";
        BOOST_THROW_EXCEPTION(keto::keystore::IndexOutOfBoundsException(ss.str()));
    }
    return this->sessionKeys[this->hashIndex[index]];

}

uint8_t NetworkSessionKeyManager::NetworkSessionSlot::getSlot() {
    return this->slot;
}

bool NetworkSessionKeyManager::NetworkSessionSlot::checkHashIndex(const std::vector<keto::rpc_protocol::NetworkKeyHelper>& networkKeyHelpers) {
    for (keto::rpc_protocol::NetworkKeyHelper networkKeyHelper: networkKeyHelpers) {
        bool found = false;
        for (std::vector<uint8_t> checkHash : this->hashIndex) {
            if (checkHash == networkKeyHelper.getHash()) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

NetworkSessionKeyManager::NetworkSessionKeyManager(const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator) :
        networkSessionGenerator(false), networkSessionConfigured(false), consensusHashGenerator(consensusHashGenerator), slot(0) {
    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();

    if (config->getVariablesMap().count(Constants::IS_NETWORK_SESSION_GENERATOR) &&
            config->getVariablesMap()[Constants::IS_NETWORK_SESSION_GENERATOR].as<std::string>().compare(Constants::IS_NETWORK_SESSION_GENERATOR_TRUE)) {
        this->networkSessionGenerator = true;
    }
    if (this->networkSessionGenerator) {
        if (config->getVariablesMap().count(Constants::NETWORK_SESSION_GENERATOR_KEYS)) {
            networkSessionKeyNumber = config->getVariablesMap()[Constants::NETWORK_SESSION_GENERATOR_KEYS].as<int>();
        } else {
            networkSessionKeyNumber = Constants::NETWORK_SESSION_GENERATOR_KEYS_DEFAULT;
        }
    }
}

NetworkSessionKeyManager::~NetworkSessionKeyManager() {
    this->sessionSlots.clear();
}

NetworkSessionKeyManagerPtr NetworkSessionKeyManager::init(
        const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator) {
    return singleton = NetworkSessionKeyManagerPtr(new NetworkSessionKeyManager(consensusHashGenerator));
}

NetworkSessionKeyManagerPtr NetworkSessionKeyManager::getInstance() {
    return singleton;
}

void NetworkSessionKeyManager::fin() {
    singleton.reset();
}

void NetworkSessionKeyManager::clearSession() {
    this->networkSessionConfigured = false;
}

void NetworkSessionKeyManager::generateSession() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (!MasterKeyManager::getInstance()->isMaster() || !this->networkSessionGenerator) {
        return;
    }
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    SessionMap sessionKeys;
    std::vector<std::vector<uint8_t>> hashIndex;
    //KETO_LOG_DEBUG << "Generate new session keys";
    for (int index = 0; index < this->networkSessionKeyNumber; index++) {
        std::shared_ptr<Botan::Private_Key> privateKey(new Botan::RSA_PrivateKey(*rng.get(), 2048));
        std::vector<uint8_t> hash = keto::crypto::SecureVectorUtils().copyFromSecure(
                keto::crypto::HashGenerator().generateHash(
                Botan::PKCS8::BER_encode(*privateKey)));
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr memoryVaultSessionKeyWrapperPtr(
                new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(privateKey));
        sessionKeys[hash] = memoryVaultSessionKeyWrapperPtr;
        hashIndex.push_back(hash);
    }
    // sort the hash index
    std::sort(hashIndex.begin(),hashIndex.end());

    // increment the lost
    this->slot++;
    if (slot >= 255) {
        this->slot = 1;
    }
    this->sessionSlots[this->slot] = NetworkSessionSlotPtr(new NetworkSessionSlot(this->slot,hashIndex,sessionKeys));
    this->slots.push_back(this->slot);

    // remove the extra slot
    popSlot();
    this->networkSessionConfigured = true;

}

void NetworkSessionKeyManager::setSession(const keto::proto::NetworkKeysWrapper& networkKeys) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(networkKeys);
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper(
        keto::crypto::SecureVectorUtils().copySecureToString(this->consensusHashGenerator->decrypt(networkKeysWrapperHelper.getBytes())));
    std::vector<keto::rpc_protocol::NetworkKeyHelper> networkKeyHelpers =  networkKeysHelper.getNetworkKeys();

    // check if the slot is registered
    if (this->sessionSlots.count(networkKeysHelper.getSlot())) {
        NetworkSessionSlotPtr networkSessionSlotPtr = this->sessionSlots[networkKeysHelper.getSlot()];
        if (networkSessionSlotPtr->checkHashIndex(networkKeyHelpers)) {
            // found match and will now ignore
            return;
        }
    }

    // setup a new slot
    SessionMap sessionKeys;
    std::vector<std::vector<uint8_t>> hashIndex;
    for (keto::rpc_protocol::NetworkKeyHelper networkKeyHelper : networkKeyHelpers) {
        Botan::DataSource_Memory datasource(networkKeyHelper.getKeyBytes_locked());
        std::shared_ptr<Botan::Private_Key> privateKey = Botan::PKCS8::load_key(datasource);
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr memoryVaultSessionKeyWrapperPtr(
                new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(privateKey));
        std::vector<uint8_t> hash = networkKeyHelper.getHash();
        if (!sessionKeys.count(hash)) {
            hashIndex.push_back(hash);
        }
        sessionKeys[hash] = memoryVaultSessionKeyWrapperPtr;
    }
    // sort the hash index
    std::sort(hashIndex.begin(),hashIndex.end());

    // setup the slot
    this->slot = networkKeysHelper.getSlot();
    this->sessionSlots[networkKeysHelper.getSlot()] = NetworkSessionSlotPtr(new NetworkSessionSlot(networkKeysHelper.getSlot(),hashIndex,sessionKeys));
    this->slots.push_back(networkKeysHelper.getSlot());

    // remove the extra slot
    popSlot();

    networkSessionConfigured = true;
}

keto::proto::NetworkKeysWrapper NetworkSessionKeyManager::getSession() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    //if (!networkSessionConfigured) {
    //    BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    //}
    if (this->sessionSlots.empty()) {
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    }
    return this->sessionSlots[this->slot]->getSession();
}


keto::event::Event NetworkSessionKeyManager::getNetworkSessionKeys(const keto::event::Event& event) {
    keto::proto::NetworkKeysWrapper networkKeysWrapper = getSession();
    return keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(networkKeysWrapper);
}

keto::event::Event NetworkSessionKeyManager::setNetworkSessionKeys(const keto::event::Event& event) {
    keto::proto::NetworkKeysWrapper networkKeysWrapper = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(event);
    setSession(networkKeysWrapper);
    return event;
}


NetworkSessionKeyEncryptorPtr NetworkSessionKeyManager::getEncryptor() {
    return NetworkSessionKeyEncryptorPtr(new NetworkSessionKeyEncryptor());
}

NetworkSessionKeyDecryptorPtr NetworkSessionKeyManager::getDecryptor() {
    return NetworkSessionKeyDecryptorPtr(new NetworkSessionKeyDecryptor());
}

int NetworkSessionKeyManager::getNumberOfKeys() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    //if (!networkSessionConfigured) {
    //    BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    //}
    if (this->sessionSlots.empty()) {
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    }
    return this->sessionSlots[this->slot]->getNumberOfKeys();
}

int NetworkSessionKeyManager::getSlot() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return this->slot;
}

keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr NetworkSessionKeyManager::getKey(int slot, int index) {
    //if (!networkSessionConfigured) {
    //    BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    //}
    if (this->sessionSlots.empty()) {
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    }
    if (!this->sessionSlots.count(slot)) {
        std::stringstream ss;
        ss << "Network slot [" << slot << "was not found";
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionKeyNotFoundException(ss.str()));
    }
    return this->sessionSlots[slot]->getKey(index);
}

void NetworkSessionKeyManager::popSlot() {
    if (this->slots.size() <= 3) {
        return;
    }
    this->sessionSlots.erase(this->slots.front());
    this->slots.pop_front();
}

}
}
