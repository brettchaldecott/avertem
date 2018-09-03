/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcPeerHelper.cpp
 * Author: ubuntu
 * 
 * Created on August 23, 2018, 5:17 PM
 */

#include "keto/router_utils/RpcPeerHelper.hpp"

#include <algorithm>

namespace keto {
namespace router_utils {

std::string RpcPeerHelper::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

RpcPeerHelper::RpcPeerHelper() {
    this->rpcPeer.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

RpcPeerHelper::RpcPeerHelper(const keto::proto::RpcPeer& orig) : rpcPeer(orig) {
    
}

RpcPeerHelper::RpcPeerHelper(const std::string& value) {
    rpcPeer.ParseFromString(value);
}

RpcPeerHelper::~RpcPeerHelper() {
}

RpcPeerHelper& RpcPeerHelper::setAccountHash(
        const keto::asn1::HashHelper& accountHash) {
    this->rpcPeer.set_account_hash(
            accountHash.operator keto::crypto::SecureVector().data(),
            accountHash.operator keto::crypto::SecureVector().size());
    return *this;
}

keto::asn1::HashHelper RpcPeerHelper::getAccountHash() {
    std::string accountHash = this->rpcPeer.account_hash();
    return keto::asn1::HashHelper(accountHash);    
}

RpcPeerHelper& RpcPeerHelper::setAccountHash(
        const std::vector<uint8_t>& accountHash) {
    this->rpcPeer.set_account_hash(
            accountHash.data(),
            accountHash.size());
    return *this;
}

std::vector<uint8_t> RpcPeerHelper::getAccountHashBytes() {
    std::string accountHash = this->rpcPeer.account_hash();
    std::vector<uint8_t> hash;
    std::copy(accountHash.begin(), accountHash.end(), std::back_inserter(hash));
    return hash;
}


std::string RpcPeerHelper::getAccountHashString() {
    return this->rpcPeer.account_hash();
}

RpcPeerHelper& RpcPeerHelper::setServer(
        const bool& server) {
    this->rpcPeer.set_server(server);
    return *this;
}

bool RpcPeerHelper::isServer() {
    return this->rpcPeer.server();
}

RpcPeerHelper& RpcPeerHelper::setPushAccount(
        const keto::proto::PushAccount& pushAccount) {
    this->rpcPeer.set_allocated_accountinfo(new keto::proto::PushAccount(pushAccount));
    return *this;
}

keto::proto::PushAccount RpcPeerHelper::getPushAccount() {
    return this->rpcPeer.accountinfo();
}

RpcPeerHelper& RpcPeerHelper::setPushAccount(
        PushAccountHelper& pushAccountHelper) {
    keto::proto::PushAccount copyPushAccount = pushAccountHelper.operator keto::proto::PushAccount();
    this->rpcPeer.set_allocated_accountinfo(new keto::proto::PushAccount(copyPushAccount));
    return *this;
}

PushAccountHelper RpcPeerHelper::getPushAccountHelper() {
    return PushAccountHelper(this->rpcPeer.accountinfo());
}

RpcPeerHelper::operator keto::proto::RpcPeer() {
    return this->rpcPeer;
}

std::string RpcPeerHelper::toString() {
    std::string result;
    this->rpcPeer.ParseFromString(result);
    return result;
}

}
}