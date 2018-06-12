/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusMessageHelper.cpp
 * Author: ubuntu
 * 
 * Created on May 28, 2018, 5:23 PM
 */


#include "keto/asn1/SignatureHelper.hpp"
#include "keto/crypto/SignatureGenerator.hpp"


#include "keto/software_consensus/Exception.hpp"
#include "keto/software_consensus/ConsensusMessageHelper.hpp"
#include "keto/software_consensus/SoftwareMerkelUtils.hpp"

namespace keto {
namespace software_consensus {

std::string ConsensusMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

ConsensusMessageHelper::ConsensusMessageHelper(
    const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) : keyLoaderPtr(keyLoaderPtr){
    this->consensusMessage.set_version(1);
}

ConsensusMessageHelper::ConsensusMessageHelper(
            const std::string& consensus) {
    if (!this->consensusMessage.ParseFromString(consensus)) {
        BOOST_THROW_EXCEPTION(keto::software_consensus::FailedParseConsensusObjectException());
    }
}

ConsensusMessageHelper::~ConsensusMessageHelper() {
}

ConsensusMessageHelper& ConsensusMessageHelper::setAccountHash(
        const std::vector<uint8_t>& accountHash) {
    this->consensusMessage.set_account_hash(accountHash.data(),accountHash.size());
    return *this;
}


ConsensusMessageHelper& ConsensusMessageHelper::setAccountHash(
        const keto::asn1::HashHelper& hashHelper) {
    this->consensusMessage.set_account_hash(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    return *this;
}

ConsensusMessageHelper& ConsensusMessageHelper::addSystemHash(
        const keto::asn1::HashHelper& hashHelper) {
    softwareMerkelUtils.addHash(hashHelper);
    this->consensusMessage.add_system_hashs(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    
    return *this;
}

ConsensusMessageHelper& ConsensusMessageHelper::generateMerkelRoot() {
    keto::asn1::HashHelper hashHelper = this->softwareMerkelUtils.computation();
    this->consensusMessage.set_instance_merkel_root(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    return *this;
}

ConsensusMessageHelper& ConsensusMessageHelper::sign() {
    keto::crypto::SignatureGenerator generator(*keyLoaderPtr);
    keto::asn1::HashHelper hashHelper;
    hashHelper = this->consensusMessage.instance_merkel_root();
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    std::vector<uint8_t> data = signatureHelper;
    this->consensusMessage.set_signature(data.data(),data.size());
    return *this;
}

ConsensusMessageHelper::operator keto::proto::ConsensusMessage() {
    return this->consensusMessage;
}

ConsensusMessageHelper::operator std::string() {
    std::string result;
    this->consensusMessage.SerializeToString(&result);
    return result;
}


}
}