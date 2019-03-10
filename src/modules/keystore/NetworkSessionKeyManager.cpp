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

#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"
#include "keto/rpc_protocol/NetworkKeyHelper.hpp"



namespace keto {
namespace keystore {

static NetworkSessionKeyManagerPtr singleton;

std::string NetworkSessionKeyManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

NetworkSessionKeyManager::NetworkSessionKeyManager(const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator) :
        networkSessionGenerator(false), consensusHashGenerator(consensusHashGenerator) {
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
    sessionKeys.clear();
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
    sessionKeys.clear();
    this->hashIndex.clear();

}

void NetworkSessionKeyManager::generateSession() {
    if (!MasterKeyManager::getInstance()->isMaster() || !this->networkSessionGenerator) {
        return;
    }
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    std::cout << "Generate new session keys" << std::endl;
    for (int index = 0; index < this->networkSessionKeyNumber; index++) {
        std::shared_ptr<Botan::Private_Key> privateKey(new Botan::RSA_PrivateKey(*rng.get(), 2048));
        std::vector<uint8_t> hash = keto::crypto::SecureVectorUtils().copyFromSecure(
                keto::crypto::HashGenerator().generateHash(
                Botan::PKCS8::BER_encode(*privateKey)));
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr memoryVaultSessionKeyWrapperPtr(
                new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(privateKey));
        this->sessionKeys[hash] = memoryVaultSessionKeyWrapperPtr;
        this->hashIndex.push_back(hash);
    }
    std::sort(this->hashIndex.begin(),this->hashIndex.end());
}

void NetworkSessionKeyManager::setSession(const keto::proto::NetworkKeysWrapper& networkKeys) {
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(networkKeys);
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper(
        keto::crypto::SecureVectorUtils().copySecureToString(this->consensusHashGenerator->decrypt(networkKeysWrapperHelper.getBytes())));
    std::vector<keto::rpc_protocol::NetworkKeyHelper> networkKeyHelpers =  networkKeysHelper.getNetworkKeys();
    for (keto::rpc_protocol::NetworkKeyHelper networkKeyHelper : networkKeyHelpers) {
        Botan::DataSource_Memory datasource(networkKeyHelper.getKeyBytes_locked());
        std::shared_ptr<Botan::Private_Key> privateKey = Botan::PKCS8::load_key(datasource);
        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr memoryVaultSessionKeyWrapperPtr(
                new keto::memory_vault_session::MemoryVaultSessionKeyWrapper(privateKey));
        std::vector<uint8_t> hash = networkKeyHelper.getHash();
        if (!this->sessionKeys.count(hash)) {
            this->hashIndex.push_back(hash);
        }
        this->sessionKeys[hash] = memoryVaultSessionKeyWrapperPtr;
    }
    std::sort(this->hashIndex.begin(),this->hashIndex.end());
}

keto::proto::NetworkKeysWrapper NetworkSessionKeyManager::getSession() {
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper;
    for (std::map<std::vector<uint8_t>,keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr>::iterator session =
            this->sessionKeys.begin(); session != this->sessionKeys.end(); ++session) {
        keto::rpc_protocol::NetworkKeyHelper networkKeyHelper;
        networkKeyHelper.setHash(session->first);
        networkKeyHelper.setKeyBytes(Botan::PKCS8::BER_encode(*session->second->getPrivateKey()));
        networkKeysHelper.addNetworkKey(networkKeyHelper);
    }

    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper;
    networkKeysWrapperHelper.setBytes(this->consensusHashGenerator->encrypt((keto::crypto::SecureVector)networkKeysHelper));
    return networkKeysWrapperHelper;
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
    return NetworkSessionKeyEncryptorPtr(new NetworkSessionKeyEncryptor(this));
}

NetworkSessionKeyDecryptorPtr NetworkSessionKeyManager::getDecryptor() {
    return NetworkSessionKeyDecryptorPtr(new NetworkSessionKeyDecryptor(this));
}

int NetworkSessionKeyManager::getNumberOfKeys() {
    return this->hashIndex.size();
}

keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr NetworkSessionKeyManager::getKey(int index) {
    if (index >= this->hashIndex.size()) {
        std::stringstream ss;
        ss << "Index out of bounds [" << index << "][" << this->hashIndex.size() << "]";
        BOOST_THROW_EXCEPTION(keto::keystore::IndexOutOfBoundsException(ss.str()));
    }
    return this->sessionKeys[this->hashIndex[index]];
}


}
}