//
// Created by Brett Chaldecott on 2019/02/01.
//

#include "keto/keystore/NetworkSessionKeyTransactionEncryptionHandler.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"
#include "keto/asn1/DeserializationHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/crypto/HashGenerator.hpp"

namespace keto {
namespace keystore {

std::string NetworkSessionKeyTransactionEncryptionHandler::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

NetworkSessionKeyTransactionEncryptionHandler::NetworkSessionKeyTransactionEncryptionHandler() {

}

NetworkSessionKeyTransactionEncryptionHandler::~NetworkSessionKeyTransactionEncryptionHandler() {

}


EncryptedDataWrapper_t* NetworkSessionKeyTransactionEncryptionHandler::encrypt(
        const TransactionMessage_t& transaction) {
    EncryptedDataWrapper_t* result = (EncryptedDataWrapper_t*)calloc(1, sizeof *result);
    result->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    keto::crypto::SecureVector bytes =
            keto::asn1::SerializationHelper<TransactionMessage>(&transaction,
                                                                &asn_DEF_TransactionMessage);

    std::vector<uint8_t> ct = NetworkSessionKeyManager::getInstance()->getEncryptor()->encrypt(bytes);

    keto::asn1::HashHelper hash(
            keto::crypto::HashGenerator().generateHash(bytes));
    Hash_t hashT = hash;
    ASN_SEQUENCE_ADD(&result->hash.list,new Hash_t(hashT));

    EncryptedData_t* encryptedData = OCTET_STRING_new_fromBuf(&asn_DEF_EncryptedData,
                                                              (const char *)ct.data(),ct.size());
    result->transaction = *encryptedData;
    free(encryptedData);

    return result;
}

TransactionMessage_t* NetworkSessionKeyTransactionEncryptionHandler::decrypt(
        const EncryptedDataWrapper_t& encrypt) {
    return keto::asn1::DeserializationHelper<TransactionMessage_t>(
            NetworkSessionKeyManager::getInstance()->getDecryptor()->decrypt(
                    copyEncryptedToVector(encrypt)),&asn_DEF_TransactionMessage).takePtr();

}


std::vector<uint8_t> NetworkSessionKeyTransactionEncryptionHandler::copyEncryptedToVector(
        const EncryptedDataWrapper_t& encrypt) {
    std::vector<uint8_t> result;
    for (int index = 0; index < encrypt.transaction.size; index++) {
        result.push_back(encrypt.transaction.buf[index]);
    }
    return result;
}


}
}
