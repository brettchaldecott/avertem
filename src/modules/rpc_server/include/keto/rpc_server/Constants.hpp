/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 10:21 AM
 */

#ifndef RPC_SERVER_CONSTANTS_HPP
#define RPC_SERVER_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace rpc_server {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    // rpc address information
    static constexpr const char* IP_ADDRESS = "rpc-server-ip-address";
    static constexpr const char* DEFAULT_IP = "0.0.0.0";
    
    static constexpr const char* EXTERNAL_IP_ADDRESS = "rpc-server-external-ip-address";
    
    static constexpr const char* PORT_NUMBER = "rpc-server-port-number";
    static constexpr const unsigned short DEFAULT_PORT_NUMBER = 28003;
    static constexpr const int MIN_PEERS = 6;
    static constexpr const int MAX_PEERS = 15;
    
    static constexpr const char* HTTP_THREADS = "rpc-server-thread-number";
    static constexpr const int DEFAULT_HTTP_THREADS = 10;

    static constexpr const int SESSION_ID_LENGTH = 32;

    static const int DEFAULT_RPC_SERVER_QUEUE_DELAY;

};
    
}
}

#endif /* CONSTANTS_HPP */

