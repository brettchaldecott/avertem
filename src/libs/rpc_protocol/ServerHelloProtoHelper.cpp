/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerHelloProtoHelper.cpp
 * Author: ubuntu
 * 
 * Created on June 18, 2018, 3:33 AM
 */

#include <algorithm>

#include "keto/asn1/SignatureHelper.hpp"
#include "keto/crypto/SignatureGenerator.hpp"

#include "keto/rpc_protocol/ServerHelloProtoHelper.hpp"

namespace keto {
namespace rpc_protocol {

ServerHelloProtoHelper::ServerHelloProtoHelper(const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) :
    keyLoaderPtr(keyLoaderPtr){
    serverHelo.set_version(1);
}

ServerHelloProtoHelper::ServerHelloProtoHelper(const std::string& value) {
    serverHelo.ParseFromString(value);
}

ServerHelloProtoHelper::~ServerHelloProtoHelper() {
}

ServerHelloProtoHelper& ServerHelloProtoHelper::setAccountHash(const std::vector<uint8_t> accountHash) {
    serverHelo.set_account_hash(accountHash.data(),accountHash.size());
    return *this;
}

std::vector<uint8_t> ServerHelloProtoHelper::getAccountHash() {
    std::vector<uint8_t> accountHash;
    std::string stringHash = serverHelo.account_hash();
    std::copy(stringHash.begin(), stringHash.end(), std::back_inserter(accountHash));
    return accountHash;
}

ServerHelloProtoHelper& ServerHelloProtoHelper::sign() {
    keto::crypto::SignatureGenerator generator(*keyLoaderPtr);
    std::vector<uint8_t> accountHash;
    std::string stringHash = serverHelo.account_hash();
    std::copy(stringHash.begin(), stringHash.end(), std::back_inserter(accountHash));
    std::vector<uint8_t> signature = generator.sign(accountHash);
    serverHelo.set_signature(signature.data(),signature.size());
    return *this;
}

ServerHelloProtoHelper::operator std::string() {
    std::string result;
    serverHelo.SerializeToString(&result);
    return result;
}


ServerHelloProtoHelper::operator keto::proto::ServerHelo() {
    return this->serverHelo;
}


}
}
