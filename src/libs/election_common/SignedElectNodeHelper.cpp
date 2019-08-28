//
// Created by Brett Chaldecott on 2019-08-27.
//

#include "keto/election_common/SignedElectNodeHelper.hpp"
#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/SignatureGenerator.hpp"



namespace keto {
namespace election_common {


std::string SignedElectNodeHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


SignedElectNodeHelper::SignedElectNodeHelper() {
    this->signedElectNode = (SignedElectNode_t*)calloc(1, sizeof *signedElectNode);
}


SignedElectNodeHelper::SignedElectNodeHelper(const SignedElectNode_t* signedElectNode) {
    this->signedElectNode = keto::asn1::clone<SignedElectNode_t>(signedElectNode,&asn_DEF_SignedElectNode);
}

SignedElectNodeHelper::SignedElectNodeHelper(const SignedElectNode_t& signedElectNode) {
    this->signedElectNode = keto::asn1::clone<SignedElectNode_t>(&signedElectNode,&asn_DEF_SignedElectNode);
}

SignedElectNodeHelper::SignedElectNodeHelper(const SignedElectNodeHelper& signedElectNodeHelper) {
    this->signedElectNode = keto::asn1::clone<SignedElectNode_t>(signedElectNodeHelper.signedElectNode,&asn_DEF_SignedElectNode);
}

SignedElectNodeHelper::~SignedElectNodeHelper() {
    if (this->signedElectNode) {
        ASN_STRUCT_FREE(asn_DEF_ElectNode, signedElectNode);
        signedElectNode = NULL;
    }
}

long SignedElectNodeHelper::getVersion() {
    return this->signedElectNode->version;
}

ElectNodeHelperPtr SignedElectNodeHelper::getElectedNode() {
    return ElectNodeHelperPtr(new ElectNodeHelper(this->signedElectNode->electedNode));
}

SignedElectNodeHelper& SignedElectNodeHelper::setElectedNode(const ElectNodeHelper& electNodeHelper) {
    this->signedElectNode->electedNode = electNodeHelper;
    this->signedElectNode->electedHash = getElectedHash();
    return *this;
}

keto::asn1::HashHelper SignedElectNodeHelper::getElectedHash() {
    ElectNodeHelper electNodeHelper(this->signedElectNode->electedNode);
    return keto::crypto::HashGenerator().generateHash((keto::crypto::SecureVector)electNodeHelper);
}

keto::asn1::SignatureHelper SignedElectNodeHelper::getSignature() {
    return this->signedElectNode->signature;
}

void SignedElectNodeHelper::sign(const keto::asn1::PrivateKeyHelper& privateKeyHelper) {
    keto::asn1::BerEncodingHelper key = privateKeyHelper.getKey();
    sign(key);
}

void SignedElectNodeHelper::sign(const keto::crypto::SecureVector& key) {
    keto::crypto::SignatureGenerator generator(key);
    keto::asn1::HashHelper hashHelper(this->signedElectNode->electedHash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    this->signedElectNode->signature = signatureHelper;
}

void SignedElectNodeHelper::sign(const keto::crypto::KeyLoaderPtr privateKey) {
    keto::crypto::SignatureGenerator generator(privateKey);
    keto::asn1::HashHelper hashHelper(this->signedElectNode->electedHash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    this->signedElectNode->signature = signatureHelper;
}

SignedElectNodeHelper::operator SignedElectNode_t*() const {
    return keto::asn1::clone<SignedElectNode_t>(this->signedElectNode,&asn_DEF_SignedElectNode);
}

SignedElectNodeHelper::operator SignedElectNode_t() const {
    SignedElectNode_t* signedElectNodePtr = keto::asn1::clone<SignedElectNode_t>(this->signedElectNode,&asn_DEF_SignedElectNode);
    SignedElectNode_t signedElectNode = *signedElectNodePtr;
    free(signedElectNodePtr);
    return signedElectNode;
}

SignedElectNode_t* SignedElectNodeHelper::getElection() {
    return keto::asn1::clone<SignedElectNode_t>(this->signedElectNode,&asn_DEF_SignedElectNode);
}

SignedElectNodeHelper::operator std::vector<uint8_t>() {
    return keto::asn1::SerializationHelper<SignedElectNode_t>(this->signedElectNode,&asn_DEF_SignedElectNode);
}

SignedElectNodeHelper::operator keto::crypto::SecureVector() {
    return keto::asn1::SerializationHelper<SignedElectNode_t>(this->signedElectNode,&asn_DEF_SignedElectNode);
}

}
}