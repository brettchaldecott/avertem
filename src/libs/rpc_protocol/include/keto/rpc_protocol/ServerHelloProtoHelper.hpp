/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerHelloProtoHelper.hpp
 * Author: ubuntu
 *
 * Created on June 18, 2018, 3:33 AM
 */

#ifndef SERVERHELOPROTOHELPER_HPP
#define SERVERHELOPROTOHELPER_HPP

#include <vector>
#include <memory>
#include <string>

#include "HandShake.pb.h"

#include "keto/crypto/KeyLoader.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace rpc_protocol {

class ServerHelloProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    ServerHelloProtoHelper(const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr);
    ServerHelloProtoHelper(const std::string& value);
    ServerHelloProtoHelper(const ServerHelloProtoHelper& orig) = default;
    virtual ~ServerHelloProtoHelper();
    
    ServerHelloProtoHelper& setAccountHash(const std::vector<uint8_t> accountHash);
    std::vector<uint8_t> getAccountHash();
    std::string getAccountHashStr();
    ServerHelloProtoHelper& sign();
    
    operator std::string();
    operator keto::proto::ServerHelo(); 
    
private:
    keto::proto::ServerHelo serverHelo;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    
};


}
}

#endif /* SERVERHELOPROTOHELPER_HPP */

