/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SoftwareConsensusHelper.cpp
 * Author: ubuntu
 * 
 * Created on June 14, 2018, 10:50 AM
 */

#include "keto/software_consensus/SoftwareConsensusHelper.hpp"


#include "keto/common/MetaInfo.hpp"
#include "keto/crypto/SignatureGenerator.hpp"
#include "keto/asn1/DeserializationHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/software_consensus/Exception.hpp"
#include "keto/software_consensus/SoftwareConsensusHelper.hpp"
#include "keto/software_consensus/SoftwareMerkelUtils.hpp"

namespace keto {
namespace software_consensus {


SoftwareConsensusHelper::SoftwareConsensusHelper() {
    softwareConsensus = (SoftwareConsensus_t*)calloc(1, sizeof *softwareConsensus);
    softwareConsensus->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    softwareConsensus->date = keto::asn1::TimeHelper();
}

SoftwareConsensusHelper::SoftwareConsensusHelper(const SoftwareConsensus_t& orig) {
    softwareConsensus = keto::asn1::clone<SoftwareConsensus_t>(&orig,&asn_DEF_SoftwareConsensus);
}

SoftwareConsensusHelper::SoftwareConsensusHelper(const std::string& orig) {
    softwareConsensus = 
            keto::asn1::DeserializationHelper<SoftwareConsensus_t>(
            (const uint8_t*)orig.data(),orig.size(),&asn_DEF_SoftwareConsensus).operator SoftwareConsensus_t*();
}

SoftwareConsensusHelper::SoftwareConsensusHelper(const SoftwareConsensusHelper& orig) {
    softwareConsensus = keto::asn1::clone<SoftwareConsensus_t>(orig.softwareConsensus,&asn_DEF_SoftwareConsensus);
}

SoftwareConsensusHelper::~SoftwareConsensusHelper() {
    if (softwareConsensus) {
        ASN_STRUCT_FREE(asn_DEF_SoftwareConsensus,softwareConsensus);
    }
}

SoftwareConsensusHelper& SoftwareConsensusHelper::setDate(
        const keto::asn1::TimeHelper& date) {
    softwareConsensus->date = date;
    return *this;
}

SoftwareConsensusHelper& SoftwareConsensusHelper::setPreviousHash(
        const keto::asn1::HashHelper& hashHelper) {
    softwareConsensus->previousHash = hashHelper;
    return *this;
}

SoftwareConsensusHelper& SoftwareConsensusHelper::setAccount(
        const keto::asn1::HashHelper& hashHelper) {
    softwareConsensus->account = hashHelper;
    return *this;
}

keto::asn1::HashHelper SoftwareConsensusHelper::getAccount() const {
    return softwareConsensus->account;
}

SoftwareConsensusHelper& SoftwareConsensusHelper::setSeed(
        const keto::asn1::HashHelper& hashHelper) {
    softwareConsensus->seed = hashHelper;
    return *this;
}

SoftwareConsensusHelper& SoftwareConsensusHelper::addSystemHash(
        const keto::asn1::HashHelper& hashHelper) {
    Hash_t hash = hashHelper.operator Hash_t();
    if (0!= ASN_SEQUENCE_ADD(&softwareConsensus->systemHashs,
            keto::asn1::clone<Hash_t>(&hash,&asn_DEF_Hash))) {
        BOOST_THROW_EXCEPTION(keto::software_consensus::FailedToAddSystemHashException());
    }
    return *this;
}


SoftwareConsensusHelper& SoftwareConsensusHelper::generateMerkelRoot() {
    SoftwareMerkelUtils softwareMerkelUtils;
    softwareMerkelUtils.addHash(this->softwareConsensus->previousHash);
    softwareMerkelUtils.addHash(this->softwareConsensus->account);
    softwareMerkelUtils.addHash(this->softwareConsensus->seed);
    
    
    for (int index = 0; index < this->softwareConsensus->systemHashs.list.count; index++) {
        softwareMerkelUtils.addHash(*this->softwareConsensus->systemHashs.list.array[index]);
    }
    
    this->softwareConsensus->merkelRoot = softwareMerkelUtils.computation();
    return *this;
}

SoftwareConsensusHelper& SoftwareConsensusHelper::sign(
            const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) {
    keto::crypto::SignatureGenerator generator(*keyLoaderPtr);
    keto::asn1::HashHelper hashHelper(this->softwareConsensus->merkelRoot);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    this->softwareConsensus->signature = signatureHelper;
    
}

SoftwareConsensusHelper::operator SoftwareConsensus_t*() {
    return keto::asn1::clone<SoftwareConsensus_t>(this->softwareConsensus,&asn_DEF_SoftwareConsensus);
}


}
}