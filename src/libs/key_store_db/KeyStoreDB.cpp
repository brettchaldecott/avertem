//
// Created by Brett Chaldecott on 2018/11/24.
//


#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/ecies.h>
#include <botan/filter.h>
#include <botan/filters.h>
#include <botan/dlies.h>
#include <botan/hex.h>


#include "keto/server_common/VectorUtils.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/key_store_db/Constants.hpp"
#include "keto/crypto/CipherBuilder.hpp"


namespace keto {
namespace key_store_db {

static KeyStoreDBPtr singleton;


std::string KeyStoreDB::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreDB::KeyStoreDB(const PrivateKeyPtr& baseKey) : baseKey(baseKey) {
    dbManagerPtr = std::shared_ptr<keto::rocks_db::DBManager>(
            new keto::rocks_db::DBManager(Constants::DB_LIST));
    keyStoreResourceManagerPtr =  KeyStoreResourceManagerPtr(
            new KeyStoreResourceManager(dbManagerPtr));


}

KeyStoreDB::~KeyStoreDB() {

}

KeyStoreDBPtr KeyStoreDB::init(const PrivateKeyPtr& baseKey) {
    return singleton = KeyStoreDBPtr(new KeyStoreDB(baseKey));
}

void KeyStoreDB::fin() {
    singleton.reset();
}


KeyStoreDBPtr KeyStoreDB::getInstance() {
    return singleton;
}


void KeyStoreDB::setValue(const keto::crypto::SecureVector& key, const keto::crypto::SecureVector& value, const OnionKeys& onionKeys) {
    rocksdb::Transaction* keyStoreTransaction = keyStoreResourceManagerPtr->getResource()->getTransaction();
    keto::rocks_db::SliceHelper keyValue(keto::crypto::SecureVectorUtils().copyFromSecure(
            key));
    rocksdb::ReadOptions readOptions;

    keto::crypto::SecureVector bytes = value;
    keto::crypto::CipherBuilder cipherBuilder(this->baseKey);
    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());

    for (PrivateKeyPtr privateKey : onionKeys) {
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));
        Botan::SymmetricKey vector = cipherBuilder.derive(32,privateKey);
        cipher->set_key(vector);
        cipher->set_iv(NULL,0);
        cipher->encrypt(bytes);
    }

    keto::rocks_db::SliceHelper valueHelper(keto::crypto::SecureVectorUtils().copyFromSecure(bytes));
    keyStoreTransaction->Put(keyValue,valueHelper);
}

void KeyStoreDB::setValue(const keto::crypto::SecureVector& key, const keto::crypto::SecureVector& value, const keto::key_store_utils::Encryptor& encryptor) {
    rocksdb::Transaction* keyStoreTransaction = keyStoreResourceManagerPtr->getResource()->getTransaction();
    keto::rocks_db::SliceHelper keyValue(keto::crypto::SecureVectorUtils().copyFromSecure(
            key));
    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper valueHelper(encryptor.encrypt(value));
    keyStoreTransaction->Put(keyValue,valueHelper);
}

void KeyStoreDB::setValue(const std::string& key, const keto::crypto::SecureVector& value, const OnionKeys& onionKeys) {
    return setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),value,onionKeys);
}

void KeyStoreDB::setValue(const std::string& key, const keto::crypto::SecureVector& value, const keto::key_store_utils::Encryptor& encryptor) {
    return setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),value,encryptor);
}

void KeyStoreDB::setValue(const std::string& key, const std::string& value, const OnionKeys& onionKeys) {
    return setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),keto::crypto::SecureVectorUtils().copyStringToSecure(value),onionKeys);
}

void KeyStoreDB::setValue(const std::string& key, const std::string& value, const keto::key_store_utils::Encryptor& encryptor) {
    return setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),keto::crypto::SecureVectorUtils().copyStringToSecure(value),encryptor);
}

bool KeyStoreDB::getValue(const keto::crypto::SecureVector& key, const OnionKeys& onionKeys, keto::crypto::SecureVector& bytes) {
    rocksdb::Transaction* keyStoreTransaction = keyStoreResourceManagerPtr->getResource()->getTransaction();
    keto::rocks_db::SliceHelper keyValue(keto::crypto::SecureVectorUtils().copyFromSecure(
            key));
    rocksdb::ReadOptions readOptions;
    std::string encrytedValues;
    auto status = keyStoreTransaction->Get(readOptions,keyValue,&encrytedValues);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() == status) {
        return false;
    }
    bytes = keto::crypto::SecureVectorUtils().copyStringToSecure(encrytedValues);

    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    keto::crypto::CipherBuilder cipherBuilder(this->baseKey);
    OnionKeys reversedKeys = onionKeys;
    std::reverse(reversedKeys.begin(), reversedKeys.end());
    for (PrivateKeyPtr privateKey : reversedKeys) {
        std::unique_ptr<Botan::StreamCipher> cipher(Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM));

        Botan::SymmetricKey vector = cipherBuilder.derive(32,privateKey);
        cipher->set_key(vector);
        cipher->set_iv(NULL,0);
        cipher->decrypt(bytes);
    }
    return true;
}

bool KeyStoreDB::getValue(const keto::crypto::SecureVector& key, const keto::key_store_utils::Decryptor& decryptor, keto::crypto::SecureVector& bytes) {
    rocksdb::Transaction* keyStoreTransaction = keyStoreResourceManagerPtr->getResource()->getTransaction();
    keto::rocks_db::SliceHelper keyValue(keto::crypto::SecureVectorUtils().copyFromSecure(
            key));
    rocksdb::ReadOptions readOptions;
    std::string encrytedValues;
    auto status = keyStoreTransaction->Get(readOptions,keyValue,&encrytedValues);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() == status) {
        return false;
    }
    bytes = decryptor.decrypt(keto::server_common::VectorUtils().copyStringToVector(encrytedValues));
    return true;
}


bool KeyStoreDB::getValue(const std::string& key, const OnionKeys& onionKeys, keto::crypto::SecureVector& bytes) {
    return getValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),onionKeys,bytes);
}

bool KeyStoreDB::getValue(const std::string& key, const keto::key_store_utils::Decryptor& decryptor, keto::crypto::SecureVector& bytes) {
    return getValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),decryptor,bytes);
}

bool KeyStoreDB::getValue(const std::string& key, const OnionKeys& onionKeys, std::string& value) {
    keto::crypto::SecureVector bytes;
    bool result = getValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),onionKeys,bytes);
    value = keto::crypto::SecureVectorUtils().copySecureToString(bytes);
    return result;
}

bool KeyStoreDB::getValue(const std::string& key, const keto::key_store_utils::Decryptor& decryptor, std::string& value) {
    keto::crypto::SecureVector bytes;
    bool result = getValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),decryptor,bytes);
    value = keto::crypto::SecureVectorUtils().copySecureToString(bytes);
    return result;
}

}
}