/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignatureVerification.cpp
 * Author: ubuntu
 * 
 * Created on February 6, 2018, 12:07 PM
 */


#include "keto/crypto/SignatureVerification.hpp"
#include "keto/crypto/Secp256K1Utils.hpp"
#include "keto/crypto/Constants.hpp"
#include "keto/crypto/BlockchainPublicKeyLoader.hpp"


namespace keto {
namespace crypto {

std::string SignatureVerification::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignatureVerification::SignatureVerificationType::SignatureVerificationType(
        const std::string& type, const Botan::Signature_Format& format) : type(type),format(format) {

}

SignatureVerification::SignatureVerificationType::~SignatureVerificationType() {

}

std::string SignatureVerification::SignatureVerificationType::getType() {
    return type;
}

Botan::Signature_Format SignatureVerification::SignatureVerificationType::getFormat() {
    return format;
}
    
SignatureVerification::SignatureVerification(std::shared_ptr<Botan::Public_Key> publicKey,
            const std::vector<uint8_t>& source) : publicKey(publicKey), source(source) {
}


SignatureVerification::SignatureVerification(const std::vector<uint8_t>& key, 
        keto::crypto::SecureVector source) : key(key) {
    for (keto::crypto::SecureVector::iterator iter = source.begin();
            iter != source.end(); iter++) {
        this->source.push_back(*iter);
    }
}

SignatureVerification::SignatureVerification(const std::vector<uint8_t>& key, 
        const std::vector<uint8_t>& source) : key(key), source(source) {
}

SignatureVerification::~SignatureVerification() {
}

bool SignatureVerification::check(const std::vector<uint8_t>& signature) {
    if (!this->publicKey) {
        this->publicKey = keto::crypto::BlockchainPublicKeyLoader(this->key).getPublicKey();
    }
    std::vector<SignatureVerification::SignatureVerificationType> signatureTypes({
        SignatureVerification::SignatureVerificationType(Constants::SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::SECP256K_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::EMSA1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA1_SIGNATURE_TYPE,Botan::IEEE_1363),

        SignatureVerification::SignatureVerificationType(Constants::EMSA3_RAW_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA3_RAW_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::EMSA3_SHA1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA3_SHA1_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::EMSA3_PKCS1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA3_PKCS1_SIGNATURE_TYPE,Botan::IEEE_1363),

        //SignatureVerification::SignatureVerificationType(Constants::EMSA4_RAW_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        //SignatureVerification::SignatureVerificationType(Constants::EMSA4_RAW_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::EMSA4_SHA1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA4_SHA1_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::EMSA4_SHA256_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA4_SHA256_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::EMSA4_SHA256_MGF1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA4_SHA256_MGF1_SIGNATURE_TYPE,Botan::IEEE_1363),

        SignatureVerification::SignatureVerificationType(Constants::PSSR_SHA256_MGF1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::PSSR_SHA256_MGF1_SIGNATURE_TYPE,Botan::IEEE_1363)

    });

    for (SignatureVerification::SignatureVerificationType signatureType: signatureTypes) {
        //KETO_LOG_DEBUG << "verify the signature using type [" << signatureType.getType() << "]["
        //    << this->source.size() << "][" << signature.size()
        //    << "][" << signatureType.getFormat() << "]";
        if (signatureType.getType() == Constants::SECP256K_SIGNATURE_TYPE) {
            if (keto::crypto::Secp256K1Utils::verifySignature(this->key,this->source,signature)) {
                return true;
            }
        } else {
            Botan::PK_Verifier verify(*this->publicKey, signatureType.getType(), signatureType.getFormat());
            //KETO_LOG_DEBUG << "Verify the message.";
            verify.update(this->source);
            if (verify.check_signature(signature)) {
                //KETO_LOG_DEBUG << "The signature is valid";
                return true;
            }
            //KETO_LOG_DEBUG << "After the message";
        }
    }
    //KETO_LOG_DEBUG << "No signature was found";
    return false;
}


}
}
