//
// Created by Brett Chaldecott on 2018/11/24.
//


#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>


#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/key_store_db/Constants.hpp"


namespace keto {
namespace key_store_db {

static KeyStoreDBPtr singleton;


std::string KeyStoreDB::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyStoreDB::KeyStoreDB() {
    dbManagerPtr = std::shared_ptr<keto::rocks_db::DBManager>(
            new keto::rocks_db::DBManager(Constants::DB_LIST));
    keyStoreResourceManagerPtr =  KeyStoreResourceManagerPtr(
            new KeyStoreResourceManager(dbManagerPtr));
}

KeyStoreDB::~KeyStoreDB() {

}

KeyStoreDBPtr KeyStoreDB::init() {
    return singleton = KeyStoreDBPtr(new KeyStoreDB());
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
    std::vector<uint8_t> encryptedBytes;
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    for (PrivateKeyPtr privateKey : onionKeys) {
        Botan::PK_Encryptor_EME enc(*privateKey,*rng.get(), Constants::ENCRYPTION_PADDING);
        encryptedBytes = enc.encrypt(bytes,*rng.get());
        bytes = keto::crypto::SecureVectorUtils().copyToSecure(encryptedBytes);
    }


    keto::rocks_db::SliceHelper valueHelper(encryptedBytes);
    keyStoreTransaction->Put(keyValue,valueHelper);
}


void KeyStoreDB::setValue(const std::string& key, const keto::crypto::SecureVector& value, const OnionKeys& onionKeys) {
    return setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),value,onionKeys);
}

void KeyStoreDB::setValue(const std::string& key, const std::string& value, const OnionKeys& onionKeys) {
    return setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),keto::crypto::SecureVectorUtils().copyStringToSecure(value),onionKeys);
}

bool KeyStoreDB::getValue(const keto::crypto::SecureVector& key, const OnionKeys& onionKeys, keto::crypto::SecureVector& bytes) {
    rocksdb::Transaction* keyStoreTransaction = keyStoreResourceManagerPtr->getResource()->getTransaction();
    keto::rocks_db::SliceHelper keyValue(keto::crypto::SecureVectorUtils().copyFromSecure(
            key));
    rocksdb::ReadOptions readOptions;
    std::string encrytedValues;
    if (rocksdb::Status::OK() != keyStoreTransaction->Get(readOptions,keyValue,&encrytedValues)) {
        return false;
    }

    bytes = keto::crypto::SecureVectorUtils().copyStringToSecure(encrytedValues);

    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    for (PrivateKeyPtr privateKey : onionKeys) {
        Botan::PK_Decryptor_EME dec(*privateKey,*rng.get(), Constants::ENCRYPTION_PADDING);
        bytes = dec.decrypt(bytes);
    }

    return true;
}


bool KeyStoreDB::getValue(const std::string& key, const OnionKeys& onionKeys, keto::crypto::SecureVector& bytes) {
    return getValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),onionKeys,bytes);
}

bool KeyStoreDB::getValue(const std::string& key, const OnionKeys& onionKeys, std::string& value) {
    keto::crypto::SecureVector bytes;
    bool result = getValue(keto::crypto::SecureVectorUtils().copyStringToSecure(key),onionKeys,bytes);
    value = keto::crypto::SecureVectorUtils().copySecureToString(bytes);
    return result;
}

}
}