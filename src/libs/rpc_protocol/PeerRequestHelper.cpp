/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerRequestHelper.cpp
 * Author: ubuntu
 * 
 * Created on August 19, 2018, 9:20 AM
 */

#include "keto/rpc_protocol/PeerRequestHelper.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "include/keto/rpc_protocol/PeerRequestHelper.hpp"

namespace keto {
namespace rpc_protocol {

std::string PeerRequestHelper::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}


PeerRequestHelper::PeerRequestHelper() {
    request.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}


PeerRequestHelper::PeerRequestHelper(const keto::proto::PeerRequest& request) : request(request) {
    
}

PeerRequestHelper::~PeerRequestHelper() {
}

PeerRequestHelper& PeerRequestHelper::addAccountHash(
       const std::vector<uint8_t> accountHash) {
    request.add_account_hash(keto::server_common::VectorUtils().copyVectorToString(accountHash));
    return *this;
}

std::vector<std::vector<uint8_t>> PeerRequestHelper::getAccountHashes() {
    std::vector<std::vector<uint8_t>> result;
    for (int index = 0; index < request.account_hash_size(); index++) {
        result.push_back(keto::server_common::VectorUtils().copyStringToVector(
                request.account_hash(index)));
    }
    return result;
}

PeerRequestHelper::operator keto::proto::PeerRequest() {
    return this->request;
}

}
}