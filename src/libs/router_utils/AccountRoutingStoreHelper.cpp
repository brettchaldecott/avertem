/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountRoutingStoreHelper.cpp
 * Author: ubuntu
 * 
 * Created on August 22, 2018, 10:14 AM
 */

#include "keto/router_utils/AccountRoutingStoreHelper.hpp"

#include <algorithm>

namespace keto {
namespace router_utils {
    
std::string AccountRoutingStoreHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountRoutingStoreHelper::AccountRoutingStoreHelper() {
    this->accountRoutingStore.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

AccountRoutingStoreHelper::AccountRoutingStoreHelper(
    const keto::proto::AccountRoutingStore& orig) : 
    accountRoutingStore(orig) {
    
}

AccountRoutingStoreHelper::AccountRoutingStoreHelper(const std::string& value) {
    this->accountRoutingStore.ParseFromString(value);
}


AccountRoutingStoreHelper::~AccountRoutingStoreHelper() {
}

AccountRoutingStoreHelper& AccountRoutingStoreHelper::setManagementAccountHash(
        const keto::asn1::HashHelper& accountHash) {
    this->accountRoutingStore.set_management_account_hash(
            accountHash.operator keto::crypto::SecureVector().data(),
            accountHash.operator keto::crypto::SecureVector().size());
    return *this;
}

keto::asn1::HashHelper AccountRoutingStoreHelper::getManagementAccountHash() {
    std::string managementAccountHash = this->accountRoutingStore.management_account_hash();
    return keto::asn1::HashHelper(managementAccountHash);
}

AccountRoutingStoreHelper& AccountRoutingStoreHelper::setManagementAccountHash(
        const std::vector<uint8_t>& accountHash) {
    this->accountRoutingStore.set_management_account_hash(
            accountHash.data(),
            accountHash.size());
    return *this;
}

std::vector<uint8_t> AccountRoutingStoreHelper::getManagementAccountHashBytes() {
    std::string accountHash = this->accountRoutingStore.management_account_hash();
    std::vector<uint8_t> hash;
    std::copy(accountHash.begin(), accountHash.end(), std::back_inserter(hash));
    return hash;
}

std::string AccountRoutingStoreHelper::toString() {
    std::string result;
    this->accountRoutingStore.SerializeToString(&result);
    return result;
}

AccountRoutingStoreHelper::operator keto::proto::AccountRoutingStore() {
    return this->accountRoutingStore;
}

}
}