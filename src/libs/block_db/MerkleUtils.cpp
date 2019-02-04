/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MerkleUtils.cpp
 * Author: ubuntu
 * 
 * Created on March 14, 2018, 7:02 AM
 */

#include <vector>

#include "keto/block_db/MerkleUtils.hpp"
#include "keto/block_db/Exception.hpp"
#include "keto/crypto/HashGenerator.hpp"
#include "include/keto/block_db/SignedChangeSetBuilder.hpp"

namespace keto {
namespace block_db {
    
std::string MerkleUtils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


MerkleUtils::MerkleUtils(std::vector<keto::asn1::HashHelper> hashs) : hashs(hashs) {
}

MerkleUtils::~MerkleUtils() {
}

keto::asn1::HashHelper MerkleUtils::computation() {
    return compute(hashs);
}

keto::asn1::HashHelper MerkleUtils::compute(std::vector<keto::asn1::HashHelper> hashs) {
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

keto::asn1::HashHelper MerkleUtils::compute(
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