/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcPeerHelper.hpp
 * Author: ubuntu
 *
 * Created on August 23, 2018, 5:17 PM
 */

#ifndef RPCPEERHELPER_HPP
#define RPCPEERHELPER_HPP

#include <string>
#include <memory>

#include "Route.pb.h"

#include "keto/asn1/HashHelper.hpp"

#include "keto/common/MetaInfo.hpp"
#include "keto/obfuscate/MetaString.hpp"

#include "keto/router_utils/PushAccountHelper.hpp"

namespace keto {
namespace router_utils {

class RpcPeerHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RpcPeerHelper();
    RpcPeerHelper(const keto::proto::RpcPeer& orig);
    RpcPeerHelper(const std::string& value);
    RpcPeerHelper(const RpcPeerHelper& orig) = default;
    virtual ~RpcPeerHelper();
    
    RpcPeerHelper& setAccountHash(
            const keto::asn1::HashHelper& accountHash);
    keto::asn1::HashHelper getAccountHash();
    RpcPeerHelper& setAccountHash(
            const std::vector<uint8_t>& accountHash);
    std::vector<uint8_t> getAccountHashBytes();
    std::string getAccountHashString();
    
    RpcPeerHelper& setServer(
            const bool& server);
    bool isServer();
    
    
    RpcPeerHelper& setPushAccount(
            const keto::proto::PushAccount& pushAccount);
    keto::proto::PushAccount getPushAccount();
    RpcPeerHelper& setPushAccount(
            PushAccountHelper& pushAccountHelper);
    PushAccountHelper getPushAccountHelper();
    
    
    operator keto::proto::RpcPeer();
    std::string toString();
    
private:
    keto::proto::RpcPeer rpcPeer;
};


}
}

#endif /* RPCPEERHELPER_HPP */

