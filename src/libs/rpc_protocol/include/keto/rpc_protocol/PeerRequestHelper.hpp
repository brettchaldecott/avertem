/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PeerRequestHelper.hpp
 * Author: ubuntu
 *
 * Created on August 19, 2018, 9:20 AM
 */

#ifndef PEERREQUESTHELPER_HPP
#define PEERREQUESTHELPER_HPP


#include <vector>
#include <memory>
#include <string>

#include "HandShake.pb.h"

#include "keto/common/MetaInfo.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace rpc_protocol {

class PeerRequestHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    
    PeerRequestHelper();
    PeerRequestHelper(const keto::proto::PeerRequest& request);
    PeerRequestHelper(const PeerRequestHelper& orig) = default;
    virtual ~PeerRequestHelper();
    
    
    PeerRequestHelper& addAccountHash(const std::vector<uint8_t> accountHash);
    std::vector<std::vector<uint8_t>> getAccountHashes();
    
    operator keto::proto::PeerRequest();
    
private:
    keto::proto::PeerRequest request;
};

}
}


#endif /* PEERREQUESTHELPER_HPP */

