/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerResponseHelper.hpp
 * Author: ubuntu
 *
 * Created on August 19, 2018, 9:21 AM
 */

#ifndef PEERRESPONSEHELPER_HPP
#define PEERRESPONSEHELPER_HPP

#include <vector>
#include <memory>
#include <string>

#include "HandShake.pb.h"

#include "keto/common/MetaInfo.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace rpc_protocol {


class PeerResponseHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    
    PeerResponseHelper();
    PeerResponseHelper(const keto::proto::PeerResponse& response);
    PeerResponseHelper(const std::string& response);
    PeerResponseHelper(const PeerResponseHelper& orig) = default;
    virtual ~PeerResponseHelper();
    
    PeerResponseHelper& addPeer(const std::string& url);
    PeerResponseHelper& addPeers(const std::vector<std::string>& url);
    std::vector<std::string> getPeers();
    
    operator keto::proto::PeerResponse();
    
private:
    keto::proto::PeerResponse response;
    
};

}
}

#endif /* PEERRESPONSEHELPER_HPP */

