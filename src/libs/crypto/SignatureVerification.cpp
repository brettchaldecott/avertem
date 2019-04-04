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
        SignatureVerification::SignatureVerificationType(Constants::SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA1_SIGNATURE_TYPE,Botan::DER_SEQUENCE),
        SignatureVerification::SignatureVerificationType(Constants::EMSA1_SIGNATURE_TYPE,Botan::IEEE_1363),
        SignatureVerification::SignatureVerificationType(Constants::SECP256K_SIGNATURE_TYPE,Botan::IEEE_1363)});

    for (SignatureVerification::SignatureVerificationType signatureType: signatureTypes) {
        std::cout << "verify the signature using type [" << signatureType.getType() << "]["
            << this->source.size() << "][" << signature.size()
            << "][" << signatureType.getFormat() << "]" << std::endl;
        if (signatureType.getType() == Constants::SECP256K_SIGNATURE_TYPE) {
            if (keto::crypto::Secp256K1Utils::verifySignature(this->key,this->source,signature)) {
                return true;
            }
        } else {
            Botan::PK_Verifier verify(*this->publicKey, signatureType.getType(), signatureType.getFormat());
            std::cout << "Verify the message." << std::endl;
            if (verify.verify_message(this->source, signature)) {
                std::cout << "The signature is valid" << std::endl;
                return true;
            }
            std::cout << "After the message" << std::endl;
        }
    }
    std::cout << "No signature was found" << std::endl;
    return false;
}


}
}