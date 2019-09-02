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
    return OBFUSCATED("$Id$");
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

std::vector<uint8_t> RpcPeerHelper::getAccountHashBytes() const {
    std::string accountHash = this->rpcPeer.account_hash();
    std::vector<uint8_t> hash;
    std::copy(accountHash.begin(), accountHash.end(), std::back_inserter(hash));
    return hash;
}


std::string RpcPeerHelper::getAccountHashString() const {
    return this->rpcPeer.account_hash();
}

RpcPeerHelper& RpcPeerHelper::setPeerAccountHash(
        const keto::asn1::HashHelper& accountHash) {
    this->rpcPeer.set_peer_account_hash(
            accountHash.operator keto::crypto::SecureVector().data(),
            accountHash.operator keto::crypto::SecureVector().size());
    return *this;
}

keto::asn1::HashHelper RpcPeerHelper::getPeerAccountHash() {
    std::string accountHash = this->rpcPeer.peer_account_hash();
    return keto::asn1::HashHelper(accountHash);
}

RpcPeerHelper& RpcPeerHelper::setPeerAccountHash(
        const std::vector<uint8_t>& accountHash) {
    this->rpcPeer.set_peer_account_hash(
            accountHash.data(),
            accountHash.size());
    return *this;
}

std::vector<uint8_t> RpcPeerHelper::getPeerAccountHashBytes() const {
    std::string accountHash = this->rpcPeer.peer_account_hash();
    std::vector<uint8_t> hash;
    std::copy(accountHash.begin(), accountHash.end(), std::back_inserter(hash));
    return hash;
}


std::string RpcPeerHelper::getPeerAccountHashString() const {
    return this->rpcPeer.peer_account_hash();
}

RpcPeerHelper& RpcPeerHelper::setServer(
        const bool& server) {
    this->rpcPeer.set_server(server);
    return *this;
}

bool RpcPeerHelper::isServer() {
    return this->rpcPeer.server();
}

RpcPeerHelper& RpcPeerHelper::addChild(
        const RpcPeerHelper& child) {
    *rpcPeer.add_rpc_children() = (keto::proto::RpcPeer)child;
    return *this;
}

RpcPeerHelper& RpcPeerHelper::addChild(
        const keto::proto::RpcPeer& child) {
    *rpcPeer.add_rpc_children() = child;
    return *this;
}

int RpcPeerHelper::numberOfChildren() const {
    return rpcPeer.rpc_children_size();
}

RpcPeerHelperPtr RpcPeerHelper::getChild(int index) const {
    if (index >= rpcPeer.rpc_children_size()) {
        return RpcPeerHelperPtr();
    }
    return RpcPeerHelperPtr(new RpcPeerHelper(rpcPeer.rpc_children(index)));
}


bool RpcPeerHelper::isActive() {
    return rpcPeer.active();
}

RpcPeerHelper& RpcPeerHelper::setActive(bool active) {
    rpcPeer.set_active(active);
    return *this;
}

RpcPeerHelper::operator keto::proto::RpcPeer() const {
    return this->rpcPeer;
}

RpcPeerHelper::operator std::string() const {
    return toString();
}

std::string RpcPeerHelper::toString() const {
    return this->rpcPeer.SerializeAsString();
}

}
}