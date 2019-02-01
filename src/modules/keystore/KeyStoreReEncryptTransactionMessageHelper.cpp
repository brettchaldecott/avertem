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


    
    return keto::asn1::DeserializationHelper<TransactionMessage_t>(
                dec.decrypt(copyEncryptedToVector(encrypt)),&asn_DEF_TransactionMessage);
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
