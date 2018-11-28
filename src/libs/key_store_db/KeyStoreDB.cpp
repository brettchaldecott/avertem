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
    for (keto::crypto::SecureVector privateKeyVector : onionKeys) {
        Botan::DataSource_Memory memoryDatasource(privateKeyVector);
        std::shared_ptr<Botan::Private_Key> privateKey =
                Botan::PKCS8::load_key(memoryDatasource);
        Botan::PK_Encryptor_EME enc(*privateKey,*rng.get(), Constants::ENCRYPTION_PADDING);
        encryptedBytes = enc.encrypt(bytes,*rng.get());
        bytes = keto::crypto::SecureVectorUtils().copyToSecure(encryptedBytes);
    }


    keto::rocks_db::SliceHelper valueHelper(encryptedBytes);
    keyStoreTransaction->Put(keyValue,valueHelper);
}


keto::crypto::SecureVector KeyStoreDB::getValue(const keto::crypto::SecureVector& key, const OnionKeys& onionKeys) {
    rocksdb::Transaction* keyStoreTransaction = keyStoreResourceManagerPtr->getResource()->getTransaction();
    keto::rocks_db::SliceHelper keyValue(keto::crypto::SecureVectorUtils().copyFromSecure(
            key));
    rocksdb::ReadOptions readOptions;
    std::string encrytedValues;
    keyStoreTransaction->Get(readOptions,keyValue,&encrytedValues);
    keto::crypto::SecureVector bytes = keto::crypto::SecureVectorUtils().copyStringToSecure(encrytedValues);

    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    for (keto::crypto::SecureVector privateKeyVector : onionKeys) {
        Botan::DataSource_Memory memoryDatasource(privateKeyVector);
        std::shared_ptr<Botan::Private_Key> privateKey =
                Botan::PKCS8::load_key(memoryDatasource);
        Botan::PK_Decryptor_EME dec(*privateKey,*rng.get(), Constants::ENCRYPTION_PADDING);


        bytes = dec.decrypt(bytes);
    }

    return bytes;
}


}
}