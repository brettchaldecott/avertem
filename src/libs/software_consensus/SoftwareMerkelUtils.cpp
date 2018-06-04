/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SoftwareMerkelUtils.cpp
 * Author: ubuntu
 * 
 * Created on May 29, 2018, 2:56 AM
 */

#include <vector>

#include "keto/software_consensus/Exception.hpp"
#include "keto/crypto/HashGenerator.hpp"
#include "keto/software_consensus/SoftwareMerkelUtils.hpp"

namespace keto {
namespace software_consensus {

SoftwareMerkelUtils::SoftwareMerkelUtils() {
}

SoftwareMerkelUtils::~SoftwareMerkelUtils() {
}

void SoftwareMerkelUtils::addHash(const keto::asn1::HashHelper& hash) {
    hashs.push_back(hash);
}

keto::asn1::HashHelper SoftwareMerkelUtils::computation() {
    return compute(sort(hashs));
}

std::vector<keto::asn1::HashHelper> SoftwareMerkelUtils::sort(
        std::vector<keto::asn1::HashHelper> hashs) {
    return hashs;
}

keto::asn1::HashHelper SoftwareMerkelUtils::compute(
        std::vector<keto::asn1::HashHelper> hashs) {
    if (!hashs.size()) {
        BOOST_THROW_EXCEPTION(keto::software_consensus::NoSoftwareHashConsensusException());
    }
    if (hashs.size() == 1) {
        return hashs[0];
    }
    if (hashs.size() % 2) {
        hashs.push_back(hashs[hashs.size() -1]);
    }
    std::vector<keto::asn1::HashHelper> parents;
    for(int index = 0; index < hashs.size();index+=2) {
        parents.push_back(compute(
            hashs[index], hashs[index+1]));
    }
    return compute(parents);
}

keto::asn1::HashHelper SoftwareMerkelUtils::compute(
        keto::asn1::HashHelper lhs, keto::asn1::HashHelper rhs) {
    keto::crypto::SecureVector bytes;
    keto::crypto::SecureVector lhsBytes = lhs;
    keto::crypto::SecureVector rhsBytes = rhs;
    bytes.insert(bytes.end(),lhsBytes.begin(),lhsBytes.end());
    bytes.insert(bytes.end(),lhsBytes.begin(),lhsBytes.end());
    return keto::crypto::HashGenerator().generateHash(bytes);
}


}
}