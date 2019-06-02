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

class RpcPeerHelper;
typedef std::shared_ptr<RpcPeerHelper> RpcPeerHelperPtr;

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
    std::vector<uint8_t> getAccountHashBytes() const;
    std::string getAccountHashString() const;

    RpcPeerHelper& setPeerAccountHash(
            const keto::asn1::HashHelper& accountHash);
    keto::asn1::HashHelper getPeerAccountHash();
    RpcPeerHelper& setPeerAccountHash(
            const std::vector<uint8_t>& accountHash);
    std::vector<uint8_t> getPeerAccountHashBytes() const;
    std::string getPeerAccountHashString() const;


    RpcPeerHelper& setServer(
            const bool& server);
    bool isServer();
    
    
    RpcPeerHelper& addChild(
            const RpcPeerHelper& child);
    RpcPeerHelper& addChild(
            const keto::proto::RpcPeer& child);
    int numberOfChildren() const;
    RpcPeerHelperPtr getChild(int index) const;
    
    operator keto::proto::RpcPeer() const;
    operator std::string() const;
    std::string toString() const;
    
private:
    keto::proto::RpcPeer rpcPeer;
};


}
}

#endif /* RPCPEERHELPER_HPP */

