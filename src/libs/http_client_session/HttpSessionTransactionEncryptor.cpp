/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpSessionTransactionEncryptor.cpp
 * Author: ubuntu
 * 
 * Created on November 6, 2018, 11:53 AM
 */


#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/rsa.h>
#include <botan/auto_rng.h>
#include <botan/stream_cipher.h>

#include "ANY.h"
#include "der_encoder.h"

#include "keto/common/MetaInfo.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/HashHelper.hpp"

#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/Constants.hpp"

#include "keto/session/Exception.hpp"
#include "keto/session/HttpSessionTransactionEncryptor.hpp"

namespace keto{
namespace session {

HttpSessionTransactionEncryptor::HttpSessionTransactionEncryptor(
    const keto::crypto::KeyLoaderPtr& keyLoaderPtr,
    const std::vector<uint8_t>& publicKey) : keyLoaderPtr(keyLoaderPtr),
    publicKey(publicKey) {
}

HttpSessionTransactionEncryptor::~HttpSessionTransactionEncryptor() {
}

EncryptedDataWrapper_t* HttpSessionTransactionEncryptor::encrypt(
        const TransactionMessage_t& transaction) {
    EncryptedDataWrapper_t* result = (EncryptedDataWrapper_t*)calloc(1, sizeof *result);
    result->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    keto::crypto::SecureVector bytes =
        keto::asn1::SerializationHelper<TransactionMessage>(&transaction,
        &asn_DEF_TransactionMessage);

    std::cout << "Attempt to load the key:" << std::endl;
    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    std::shared_ptr<Botan::Public_Key> publicKey(
             Botan::X509::load_key(this->publicKey));

    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM);
    keto::crypto::SecureVector secureVector = rng->random_vec(cipher->key_spec().maximum_keylength());

    cipher->set_key(Botan::SymmetricKey(secureVector));
    cipher->set_iv(NULL,0);
    cipher->encrypt(bytes);

    Botan::PK_Encryptor_EME enc(*publicKey,*rng.get(), keto::crypto::Constants::ENCRYPTION_PADDING);
    std::vector<uint8_t> ct = enc.encrypt(secureVector,*rng.get());
    
    keto::asn1::HashHelper hash(
        keto::crypto::HashGenerator().generateHash(bytes));
    Hash_t hashT = hash;
    ASN_SEQUENCE_ADD(&result->hash.list,new Hash_t(hashT));

    std::cout << "prepend the encrypted cypher : " << publicKey->message_part_size() << std::endl;
    std::cout << "Retrieve the key length : " << (publicKey->key_length() / 8) << std::endl;
    std::cout << "prepend the encrypted cypher : " << ct.size() << std::endl;
    bytes.insert(bytes.begin(),ct.begin(),ct.end());
    std::cout << "The size of the bytes : " << bytes.size() << std::endl;

    std::cout << "Setup the encrypted bytes" << std::endl;
    EncryptedData_t* encryptedData = OCTET_STRING_new_fromBuf(&asn_DEF_EncryptedData,
            (const char *)bytes.data(),bytes.size());
    result->transaction = *encryptedData;
    free(encryptedData);
    
    return result;
}

TransactionMessage_t* HttpSessionTransactionEncryptor::decrypt(
        const EncryptedDataWrapper_t& encrypt) {
    BOOST_THROW_EXCEPTION(keto::session::UnsupportedOperationException());
}


}
}
