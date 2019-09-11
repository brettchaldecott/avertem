//
// Created by Brett Chaldecott on 2019/01/28.
//

#include "keto/server_common/EventUtils.hpp"
#include "keto/keystore/MasterKeyManager.hpp"
#include "keto/keystore/MasterKeyManagerMisc.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/keystore/Exception.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/crypto/KeyBuilder.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/keystore/KeyStoreStorageManager.hpp"
#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"


namespace keto {
namespace keystore {

static MasterKeyManagerPtr singleton;

std::string MasterKeyManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

MasterKeyManager::MasterKeyListEntry::MasterKeyListEntry() {

}

MasterKeyManager::MasterKeyListEntry::MasterKeyListEntry(const std::string& jsonString) {
    //KETO_LOG_DEBUG << "The json string : " << jsonString;
    nlohmann::json jsonEntry = nlohmann::json::parse(jsonString);
    for (nlohmann::json::iterator it = jsonEntry.begin(); it != jsonEntry.end(); ++it) {
        this->keys.push_back(*it);
    }
}

MasterKeyManager::MasterKeyListEntry::~MasterKeyListEntry() {

}

void MasterKeyManager::MasterKeyListEntry::addKey(const std::string& hash) {
    this->keys.push_back(hash);
}

std::vector<std::string> MasterKeyManager::MasterKeyListEntry::getKeys() {
    return this->keys;
}

std::string MasterKeyManager::MasterKeyListEntry::getJson() {
    nlohmann::json json;
    for (std::string hash : this->keys) {
        json.push_back(hash);
    }
    return json.dump();
}

void MasterKeyManager::Session::initSession() {
    KeyStoreWrapIndexManager::getInstance()->initSession();
    KeyStoreStorageManager::getInstance()->initSession();
}

void MasterKeyManager::Session::clearSession() {
    KeyStoreStorageManager::getInstance()->clearSession();
    KeyStoreWrapIndexManager::getInstance()->clearSession();
}

// master session
MasterKeyManager::MasterSession::MasterSession(std::shared_ptr<keto::environment::Config> config) {
    std::string password =
            config->getVariablesMap()[Constants::MASTER_PASSWORD].as<std::string>();
    bool result1 = authenticateMaster(password);
    bool result2 = authenticateMaster(Constants::FALSE_MASTER_PASSWORD);
    bool result3 = authenticateMaster(password);
    if (!(result1 && !result2 && result3)) {
        BOOST_THROW_EXCEPTION(keto::keystore::InvalidPasswordForMasterException());
    }

    this->keyStoreDBPtr = keto::key_store_db::KeyStoreDB::getInstance();

    // load the key information
    if (!config->getVariablesMap().count(Constants::MASTER_PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::keystore::MasterPrivateKeyNotConfiguredException());
    }
    std::string privateKeyPath =
            config->getVariablesMap()[Constants::MASTER_PRIVATE_KEY].as<std::string>();

    if (!config->getVariablesMap().count(Constants::MASTER_PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::keystore::MasterPublicKeyNotConfiguredException());
    }
    std::string publicKeyPath =
            config->getVariablesMap()[Constants::MASTER_PUBLIC_KEY].as<std::string>();

    this->masterKeyLock = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
                                                                    publicKeyPath);

}

MasterKeyManager::MasterSession::~MasterSession() {

}

void MasterKeyManager::MasterSession::initSession() {
    initMasterKeys();

    try {
        //KETO_LOG_DEBUG << "Init the session";
        Session::initSession();

        keto::event::Event event;

        //KETO_LOG_DEBUG << "Set the master key";
        setMasterKey(event);
        //KETO_LOG_DEBUG << "Set the wrapping keys";
        setWrappingKeys(event);
        //KETO_LOG_DEBUG << "After setting the wrapping keys";
    }  catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[initSession]Failed to init the session : " << ex.what();
        KETO_LOG_ERROR << "[initSession]Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[initSession]Failed to init the session";
        KETO_LOG_ERROR << "[initSession]Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[initSession]Failed to init the session";
        KETO_LOG_ERROR << "[initSession]The cause is : " << ex.what();
        throw;
    } catch (...) {
        KETO_LOG_ERROR << "[initSession]Failed to init the session";
        throw;
    }
}

void MasterKeyManager::MasterSession::clearSession() {
    Session::clearSession();

    this->masterWrapperKeyList.reset();
    this->masterKeyList.reset();
}

bool MasterKeyManager::MasterSession::isMaster() const {
    return true;
}

keto::event::Event MasterKeyManager::MasterSession::getMasterKey(const keto::event::Event& event) {
    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->masterKeyLock->getPrivateKey());
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper;
    this->loadKeys(this->masterKeyList,networkKeysHelper,onionKeys);
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper;
    networkKeysWrapperHelper.setBytes(NetworkSessionKeyManager::getInstance()->getEncryptor()->encrypt((keto::crypto::SecureVector)networkKeysHelper));
    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    return keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(networkKeysWrapper);
}

keto::event::Event MasterKeyManager::MasterSession::setMasterKey(const keto::event::Event& event) {
    // ignore this
    //KETO_LOG_DEBUG << "Load the onions";
    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->masterKeyLock->getPrivateKey());
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper;
    //KETO_LOG_DEBUG << "load the keys";
    this->loadKeys(this->masterKeyList,networkKeysHelper,onionKeys);
    //KETO_LOG_DEBUG << "Set the keys";
    KeyStoreWrapIndexManager::getInstance()->setMasterKey(networkKeysHelper);
    //KETO_LOG_DEBUG << "Return the keys";
    return event;
}

keto::event::Event MasterKeyManager::MasterSession::getWrappingKeys(const keto::event::Event& event) {
    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->masterKeyLock->getPrivateKey());
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper;
    this->loadKeys(this->masterWrapperKeyList,networkKeysHelper,onionKeys);
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper;
    networkKeysWrapperHelper.setBytes(NetworkSessionKeyManager::getInstance()->getEncryptor()->encrypt((keto::crypto::SecureVector)networkKeysHelper));
    keto::proto::NetworkKeysWrapper networkKeysWrapper = networkKeysWrapperHelper;
    return keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(networkKeysWrapper);
}

keto::event::Event MasterKeyManager::MasterSession::setWrappingKeys(const keto::event::Event& event) {
    // ignore this
    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->masterKeyLock->getPrivateKey());
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper;
    this->loadKeys(this->masterWrapperKeyList,networkKeysHelper,onionKeys);
    //KETO_LOG_DEBUG << "Set the wrapping keys";
    KeyStoreWrapIndexManager::getInstance()->setWrappingKeys(networkKeysHelper);
    //KETO_LOG_DEBUG << "After setting the wrapping keys";
    return event;
}

void MasterKeyManager::MasterSession::initMasterKeys() {

    keto::key_store_db::OnionKeys onionKeys;
    onionKeys.push_back(KeyStoreStorageManager::getInstance()->getKeyLoader()->getPrivateKey());
    onionKeys.push_back(this->masterKeyLock->getPrivateKey());

    try {
        std::string value;
        //KETO_LOG_DEBUG << "Get value";
        if (!this->keyStoreDBPtr->getValue(Constants::KEY_STORE_DB::KEY_STORE_MASTER_ENTRY, onionKeys, value)) {
            //KETO_LOG_DEBUG << "Generate the keys";
            this->masterKeyList = generateKeys(1, onionKeys);
            //KETO_LOG_DEBUG << "Set the value value";
            this->keyStoreDBPtr->setValue(Constants::KEY_STORE_DB::KEY_STORE_MASTER_ENTRY,
                                          this->masterKeyList->getJson(), onionKeys);
        } else {
            //KETO_LOG_DEBUG << "Load the master key value";
            this->masterKeyList = MasterKeyListEntryPtr(new MasterKeyListEntry(value));
        }
        //KETO_LOG_DEBUG << "Load the wrapper keys";

        if (!this->keyStoreDBPtr->getValue(Constants::KEY_STORE_DB::KEY_STORE_WRAPPER_ENTRY, onionKeys, value)) {
            KETO_LOG_DEBUG << "Generate the wrapper keys";
            this->masterWrapperKeyList = generateKeys(Constants::KEY_STORE_DB::KEY_STORE_WRAPPER_SIZE, onionKeys);
            KETO_LOG_DEBUG << "Set the wrapper values";
            this->keyStoreDBPtr->setValue(Constants::KEY_STORE_DB::KEY_STORE_WRAPPER_ENTRY,
                                          this->masterWrapperKeyList->getJson(), onionKeys);
        } else {
            KETO_LOG_DEBUG << "Load the wrapper values";
            this->masterWrapperKeyList = MasterKeyListEntryPtr(new MasterKeyListEntry(value));
        }
        //KETO_LOG_DEBUG << "Finished initing the keys";
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[initMasterKeys]Failed to add the master : " << ex.what();
        KETO_LOG_ERROR << "[initMasterKeys]Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[initMasterKeys]Failed to add the master";
        KETO_LOG_ERROR << "[initMasterKeys]Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[initMasterKeys]Failed to add the master";
        KETO_LOG_ERROR << "[initMasterKeys]The cause is : " << ex.what();
        throw;
    } catch (...) {
        KETO_LOG_ERROR << "[initMasterKeys]Failed to add the master";
        throw;
    }
}

MasterKeyManager::MasterKeyListEntryPtr MasterKeyManager::MasterSession::generateKeys(size_t number,keto::key_store_db::OnionKeys onionKeys) {
    MasterKeyListEntryPtr result(new MasterKeyListEntry());
    for (int index = 0; index < number; index++) {
        KeyStoreEntry keyStoreEntry;
        std::string id = Botan::hex_encode(keyStoreEntry.getHash(),true);
        this->keyStoreDBPtr->setValue(id,keyStoreEntry.getJson(),onionKeys);
        result->addKey(id);
    }
    return result;
}

void MasterKeyManager::MasterSession::loadKeys(MasterKeyManager::MasterKeyListEntryPtr keyList,
        keto::rpc_protocol::NetworkKeysHelper& networkKeysHelper, keto::key_store_db::OnionKeys& onionKeys) {
    //KETO_LOG_DEBUG << "The list of keys";
    for (std::string key : keyList->getKeys()) {
        std::string value;
        //KETO_LOG_DEBUG << "Get the key value";
        if (!this->keyStoreDBPtr->getValue(key,onionKeys,value)) {
            BOOST_THROW_EXCEPTION(keto::keystore::UnknownKeyException());
        }
        //KETO_LOG_DEBUG << "Load the key : " << value;
        KeyStoreEntry keyStoreEntry(value);
        keto::rpc_protocol::NetworkKeyHelper networkKeyHelper;
        //KETO_LOG_DEBUG << "Set the hash";
        networkKeyHelper.setHash(keyStoreEntry.getHash());
        //KETO_LOG_DEBUG << "Load the key : " << value;
        networkKeyHelper.setKeyBytes(Botan::PKCS8::BER_encode(*keyStoreEntry.getPrivateKey()));
        //KETO_LOG_DEBUG << "Get the active key";
        networkKeyHelper.setActive(keyStoreEntry.getActive());
        //KETO_LOG_DEBUG << "The network key helper";
        networkKeysHelper.addNetworkKey(networkKeyHelper);
    }
    //KETO_LOG_DEBUG << "After the keys";
}



MasterKeyManager::SlaveSession::SlaveSession() : slaveMaster(false), slaveWrapper(false) {

}

MasterKeyManager::SlaveSession::~SlaveSession() {
}

bool MasterKeyManager::SlaveSession::isMaster() const {
    return false;
}

// methods to get and set the master key
keto::event::Event MasterKeyManager::SlaveSession::getMasterKey(const keto::event::Event& event) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (!this->slaveMaster) {
        KETO_LOG_ERROR << "[MasterKeyManager][SlaveSession][getMasterKey] the slave masters keys have not been set";
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    }
    return keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(this->slaveMasterKeys);
}

keto::event::Event MasterKeyManager::SlaveSession::setMasterKey(const keto::event::Event& event) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_INFO << "[MasterKeyManager][SlaveSession][setMasterKey] set up the slave master key";
    this->slaveMaster = true;
    this->slaveMasterKeys = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(event);
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(this->slaveMasterKeys);
    keto::crypto::SecureVector bytes =
            NetworkSessionKeyManager::getInstance()->getDecryptor()->decrypt(networkKeysWrapperHelper);
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    KeyStoreWrapIndexManager::getInstance()->setMasterKey(networkKeysHelper);
    return event;
}


// this method returns the wrapping keys
keto::event::Event MasterKeyManager::SlaveSession::getWrappingKeys(const keto::event::Event& event) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_INFO << "[MasterKeyManager][SlaveSession][getWrappingKeys] get the wrapping keys";
    if (!this->slaveWrapper) {
        KETO_LOG_ERROR << "[MasterKeyManager][SlaveSession][getWrappingKeys] the slave wrapper keys have not been set";
        BOOST_THROW_EXCEPTION(keto::keystore::NetworkSessionNotStartedException());
    }
    return keto::server_common::toEvent<keto::proto::NetworkKeysWrapper>(this->slaveWrapperKeys);
}

keto::event::Event MasterKeyManager::SlaveSession::setWrappingKeys(const keto::event::Event& event) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    KETO_LOG_INFO << "[MasterKeyManager][SlaveSession][setWrappingKeys] set up the slave wrapping keys";
    this->slaveWrapper = true;
    this->slaveWrapperKeys = keto::server_common::fromEvent<keto::proto::NetworkKeysWrapper>(event);
    keto::rpc_protocol::NetworkKeysWrapperHelper networkKeysWrapperHelper(this->slaveWrapperKeys);
    keto::crypto::SecureVector bytes =
            NetworkSessionKeyManager::getInstance()->getDecryptor()->decrypt(networkKeysWrapperHelper);
    keto::rpc_protocol::NetworkKeysHelper networkKeysHelper(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    KeyStoreWrapIndexManager::getInstance()->setWrappingKeys(networkKeysHelper);
    return event;
}


MasterKeyManager::MasterKeyManager() {
    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (config->getVariablesMap().count(Constants::IS_MASTER) &&
        config->getVariablesMap()[Constants::IS_MASTER].as<std::string>().compare(Constants::IS_MASTER_TRUE)) {
        this->sessionPtr = SessionPtr(new MasterSession(config));
    } else {
        this->sessionPtr = SessionPtr(new SlaveSession());
    }

}

MasterKeyManager::~MasterKeyManager() {

}

MasterKeyManagerPtr MasterKeyManager::init() {
    return singleton = MasterKeyManagerPtr(new MasterKeyManager());
}

void MasterKeyManager::fin() {
    singleton.reset();

}

MasterKeyManagerPtr MasterKeyManager::getInstance() {
    return singleton;
}

void MasterKeyManager::initSession() {
    this->sessionPtr->initSession();
}

void MasterKeyManager::clearSession() {
    this->sessionPtr->clearSession();
}


// methods to get and set the master key
keto::event::Event MasterKeyManager::getMasterKey(const keto::event::Event& event) {
    return this->sessionPtr->getMasterKey(event);

}

keto::event::Event MasterKeyManager::setMasterKey(const keto::event::Event& event) {
    return this->sessionPtr->setMasterKey(event);
}


// this method returns the wrapping keys
keto::event::Event MasterKeyManager::getWrappingKeys(const keto::event::Event& event) {
    return this->sessionPtr->getWrappingKeys(event);
}

keto::event::Event MasterKeyManager::setWrappingKeys(const keto::event::Event& event) {
    return this->sessionPtr->setWrappingKeys(event);
}

/**
 * This method returns the master reference.
 *
 * @return TRUE if this is the master
 */
bool MasterKeyManager::isMaster() const {
    return this->sessionPtr->isMaster();
}


keto::event::Event MasterKeyManager::isMaster(const keto::event::Event& event) const {
    keto::proto::MasterInfo masterInfo = keto::server_common::fromEvent<keto::proto::MasterInfo>(event);
    masterInfo.set_is_master(this->sessionPtr->isMaster());
    return keto::server_common::toEvent<keto::proto::MasterInfo>(masterInfo);
}


}
}
