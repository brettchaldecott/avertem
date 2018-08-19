/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerResponseHelper.cpp
 * Author: ubuntu
 * 
 * Created on August 19, 2018, 9:21 AM
 */

#include "keto/rpc_protocol/PeerResponseHelper.hpp"

namespace keto {
namespace rpc_protocol {

std::string PeerResponseHelper::getSourceVersion() {
    return OBFUSCATED("$Id:$");
}

    
PeerResponseHelper::PeerResponseHelper() {
    response.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

PeerResponseHelper::PeerResponseHelper(const keto::proto::PeerResponse& response) :
    response(response) {
}

PeerResponseHelper::~PeerResponseHelper() {
}

PeerResponseHelper& PeerResponseHelper::addPeer(const std::string& url) {
    response.add_peers(url);
    return *this;
}

PeerResponseHelper& PeerResponseHelper::addPeers(const std::vector<std::string>& urls) {
    for(std::string url : urls) {
        response.add_peers(url);
    }
    return *this;
}

std::vector<std::string> PeerResponseHelper::getPeers() {
    std::vector<std::string> result;
    for (int index = 0; index < response.peers_size(); index++) {
        result.push_back(
                response.peers(index));
    }
    return result;
}

PeerResponseHelper::operator keto::proto::PeerResponse() {
    return response;
}


}
}