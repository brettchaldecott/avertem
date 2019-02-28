//
//  KeyStoreReEncryptTransactionMessageHelper.cpp
//  keto_asn1_common
//
//  Created by Brett Chaldecott on 2018/11/22.
//

#include <memory>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/stream_cipher.h>


#include "keto/keystore/KeyStoreReEncryptTransactionMessageHelper.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/asn1/DeserializationHelper.hpp"

namespace keto {
namespace keystore {
    

KeyStoreReEncryptTransactionMessageHelper::KeyStoreReEncryptTransactionMessageHelper(
        const keto::crypto::SecureVector& privateKey) : privateKey(privateKey) {
    
}


KeyStoreReEncryptTransactionMessageHelper::~KeyStoreReEncryptTransactionMessageHelper() {
    
}


EncryptedDataWrapper_t* KeyStoreReEncryptTransactionMessageHelper::encrypt(
                                const TransactionMessage_t& transaction) {

    return NULL;
}

TransactionMessage_t* KeyStoreReEncryptTransactionMessageHelper::decrypt(
        const EncryptedDataWrapper_t& encrypt) {
    
    Botan::DataSource_Memory memoryDatasource(privateKey);
    std::shared_ptr<Botan::Private_Key> privateKey =
            Botan::PKCS8::load_key(memoryDatasource);

    std::unique_ptr<Botan::RandomNumberGenerator> rng(new Botan::AutoSeeded_RNG);
    Botan::PK_Decryptor_EME dec(*privateKey,*rng.get(), Constants::ENCRYPTION_PADDING);

    keto::crypto::SecureVector encrypted = copyEncryptedToVector(encrypt);


    std::cout << "The encrypted data :" << encrypted.size() << std::endl;
    std::cout << "Retrieve the necessary data : " << (privateKey->key_length() / 8) << std::endl;
    int messageSize = (privateKey->key_length() / 8);
    keto::crypto::SecureVector encryptedCipher(&encrypted[0],&encrypted[messageSize]);
    keto::crypto::SecureVector encryptedValue(&encrypted[messageSize],&encrypted[encrypted.size()]);

    std::cout << "decrypt" << std::endl;
    keto::crypto::SecureVector cipherVector = dec.decrypt(encryptedCipher);

    std::cout << "encrypt" << std::endl;
    std::unique_ptr<Botan::StreamCipher> cipher = Botan::StreamCipher::create(keto::crypto::Constants::CIPHER_STREAM);
    cipher->set_key(Botan::SymmetricKey(cipherVector));
    cipher->set_iv(NULL,0);
    cipher->encrypt(encryptedValue);

    std::cout << "Deserialize the transaction" << std::endl;
    return keto::asn1::DeserializationHelper<TransactionMessage_t>(
                encryptedValue,&asn_DEF_TransactionMessage).takePtr();
}


keto::crypto::SecureVector KeyStoreReEncryptTransactionMessageHelper::copyEncryptedToVector(
        const EncryptedDataWrapper_t& encrypt) {
    keto::crypto::SecureVector result;
    for (int index = 0; index < encrypt.transaction.size; index++) {
        result.push_back(encrypt.transaction.buf[index]);
    }
    return result;
}

}
}
