/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PushAccountHelper.cpp
 * Author: ubuntu
 * 
 * Created on August 22, 2018, 6:51 PM
 */

#include "keto/router_utils/PushAccountHelper.hpp"

#include <algorithm>

namespace keto {
namespace router_utils {

std::string PushAccountHelper::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

PushAccountHelper::PushAccountHelper() {
    this->pushAccount.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

PushAccountHelper::PushAccountHelper(const keto::proto::PushAccount& orig) :
    pushAccount(orig) {
    
}

PushAccountHelper::PushAccountHelper(const std::string& value) {
    this->pushAccount.ParseFromString(value);
}

PushAccountHelper::~PushAccountHelper() {
}

PushAccountHelper& PushAccountHelper::setAccountHash(
        const keto::asn1::HashHelper& accountHash) {
    this->pushAccount.set_account_hash(
            accountHash.operator keto::crypto::SecureVector().data(),
            accountHash.operator keto::crypto::SecureVector().size());
    return *this;
}

keto::asn1::HashHelper PushAccountHelper::getAccountHash() {
    std::string accountHash = this->pushAccount.account_hash();
    return keto::asn1::HashHelper(accountHash);
}

PushAccountHelper& PushAccountHelper::setAccountHash(
        const std::vector<uint8_t>& accountHash) {
    this->pushAccount.set_account_hash(
            accountHash.data(),
            accountHash.size());
    return *this;
}

std::vector<uint8_t> PushAccountHelper::getAccountHashBytes() {
    std::string accountHash = this->pushAccount.account_hash();
    std::vector<uint8_t> hash;
    std::copy(accountHash.begin(), accountHash.end(), std::back_inserter(hash));
    return hash;
}

PushAccountHelper& PushAccountHelper::setAccountHash(
        const std::string& accountHash) {
    this->pushAccount.set_account_hash(accountHash);
    return *this;
}

std::string PushAccountHelper::getAccountHashString() {
    return this->pushAccount.account_hash();
}

PushAccountHelper& PushAccountHelper::setManagementAccountHash(
        const keto::asn1::HashHelper& accountHash) {
    this->pushAccount.set_management_account_hash(
            accountHash.operator keto::crypto::SecureVector().data(),
            accountHash.operator keto::crypto::SecureVector().size());
    return *this;
}

keto::asn1::HashHelper PushAccountHelper::getManagementAccountHash() {
    std::string accountHash = this->pushAccount.management_account_hash();
    return keto::asn1::HashHelper(accountHash);
}

PushAccountHelper& PushAccountHelper::setManagementAccountHash(
        const std::vector<uint8_t>& accountHash) {
    this->pushAccount.set_management_account_hash(
            accountHash.data(),
            accountHash.size());
    return *this;
}

std::vector<uint8_t> PushAccountHelper::getManagementAccountHashBytes() {
    std::string accountHash = this->pushAccount.management_account_hash();
    std::vector<uint8_t> hash;
    std::copy(accountHash.begin(), accountHash.end(), std::back_inserter(hash));
    
    return hash;
}

PushAccountHelper& PushAccountHelper::setManagementAccountHash(
        const std::string& accountHash) {
    this->pushAccount.set_management_account_hash(accountHash);
    return *this;
}

std::string PushAccountHelper::getManagementAccountHashString() {
    return this->pushAccount.management_account_hash();
}


PushAccountHelper& PushAccountHelper::addChildAccount(
        const keto::proto::PushAccount& child) {
    
    this->pushAccount.mutable_child_accounts()->AddAllocated(
            new keto::proto::PushAccount(child));
    return *this;
}

PushAccountHelper& PushAccountHelper::addChildAccount(
        PushAccountHelper& child) {
    keto::proto::PushAccount childCopy = (keto::proto::PushAccount)child;
    this->pushAccount.mutable_child_accounts()->AddAllocated(
            new keto::proto::PushAccount(
            childCopy));
    return *this;
}

std::vector<PushAccountHelper> PushAccountHelper::getChildren() {
    std::vector<PushAccountHelper> result;
    for(int index = 0; index < this->pushAccount.child_accounts_size(); index++) {
        result.push_back(PushAccountHelper(this->pushAccount.child_accounts(index)));
    }
    return result;
}


PushAccountHelper::operator keto::proto::PushAccount() {
    return this->pushAccount;
}

std::string PushAccountHelper::toString() {
    std::string result;
    this->pushAccount.SerializePartialToString(&result);
    return result;
}

}
}