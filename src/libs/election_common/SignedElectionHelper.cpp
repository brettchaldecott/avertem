//
// Created by Brett Chaldecott on 2019-08-22.
//

#include "keto/election_common/SignedElectionHelper.hpp"
#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/SignatureGenerator.hpp"

#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/asn1/DeserializationHelper.hpp"

namespace keto {
namespace election_common {

std::string SignedElectionHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignedElectionHelper::SignedElectionHelper() {
    this->signedElection = (SignedElection_t*)calloc(1, sizeof *signedElection);
}

SignedElectionHelper::SignedElectionHelper(const SignedElection_t* signedElection) {
    this->signedElection = keto::asn1::clone<SignedElection_t>(signedElection,&asn_DEF_SignedElection);
}

SignedElectionHelper::SignedElectionHelper(const SignedElection_t& signedElection) {
    this->signedElection = keto::asn1::clone<SignedElection_t>(&signedElection,&asn_DEF_SignedElection);
}

SignedElectionHelper::SignedElectionHelper(const std::string& signedElection) {
    this->signedElection =
            keto::asn1::DeserializationHelper<SignedElection_t>(
                    keto::server_common::VectorUtils().copyStringToVector(signedElection),&asn_DEF_SignedElection).takePtr();
}

SignedElectionHelper::SignedElectionHelper(const SignedElectionHelper& signedElectionHelper) {
    this->signedElection = keto::asn1::clone<SignedElection_t>(signedElectionHelper.signedElection,&asn_DEF_SignedElection);
}

SignedElectionHelper::~SignedElectionHelper() {
    if (this->signedElection) {
        ASN_STRUCT_FREE(asn_DEF_SignedElection, signedElection);
        signedElection = NULL;
    }
}

long SignedElectionHelper::getVersion() {
    return this->signedElection->version;
}

SignedElectionHelper& SignedElectionHelper::setElectionHelper(const ElectionHelper& electionHelper) {
    this->signedElection->election = electionHelper;
    this->signedElection->electionHash = getElectionHash();
    return *this;
}

ElectionHelperPtr SignedElectionHelper::getElectionHelper() {
    return ElectionHelperPtr(new ElectionHelper(this->signedElection->election));
}

keto::asn1::HashHelper SignedElectionHelper::getElectionHash() {
    ElectionHelper electionHelper(this->signedElection->election);
    return keto::crypto::HashGenerator().generateHash((keto::crypto::SecureVector)electionHelper);
}

keto::asn1::SignatureHelper SignedElectionHelper::getSignature() {
    return this->signedElection->signature;
}

void SignedElectionHelper::sign(const keto::asn1::PrivateKeyHelper& privateKeyHelper) {
    keto::asn1::BerEncodingHelper key = privateKeyHelper.getKey();
    sign(key);
}

void SignedElectionHelper::sign(const keto::crypto::SecureVector& key) {
    keto::crypto::SignatureGenerator generator(key);
    keto::asn1::HashHelper hashHelper(this->signedElection->electionHash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    this->signedElection->signature = signatureHelper;
}

void SignedElectionHelper::sign(const keto::crypto::KeyLoaderPtr privateKey) {
    keto::crypto::SignatureGenerator generator(privateKey);
    keto::asn1::HashHelper hashHelper(this->signedElection->electionHash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    this->signedElection->signature = signatureHelper;
}

SignedElectionHelper::operator std::vector<uint8_t>() const {
    return keto::asn1::SerializationHelper<SignedElection_t>(this->signedElection,&asn_DEF_SignedElection);
}

SignedElectionHelper::operator keto::crypto::SecureVector() const {
    return keto::asn1::SerializationHelper<SignedElection_t>(this->signedElection,&asn_DEF_SignedElection);
}

SignedElectionHelper::operator std::string() const {
    return keto::server_common::VectorUtils().copyVectorToString(
            keto::asn1::SerializationHelper<SignedElection_t>(this->signedElection,&asn_DEF_SignedElection));
}

SignedElectionHelper::operator SignedElection_t*() const {
    return keto::asn1::clone<SignedElection_t>(this->signedElection,&asn_DEF_SignedElection);
}

SignedElectionHelper::operator SignedElection_t() const {
    SignedElection_t* signedElectionPtr = keto::asn1::clone<SignedElection_t>(this->signedElection,&asn_DEF_SignedElection);
    SignedElection_t signedElection = *signedElectionPtr;
    free(signedElectionPtr);
    return signedElection;
}

}
}