/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 3:36 PM
 */

#ifndef RPC_CLIENT_CONSTANTS_HPP
#define RPC_CLIENT_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace rpc_client {


class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    static constexpr const char* DEFAULT_PORT_NUMBER = "28003";
    
    static constexpr const char* PEERS = "rpc-peer";
    
    static constexpr const char* PEER_HELLO = "HELLO";
    
    static constexpr const char* RPC_CLIENT_THREADS = "rpc-server-thread-number";
    static constexpr const int DEFAULT_RPC_CLIENT_THREADS = 10;
    
    // keys for server
    static constexpr const char* PRIVATE_KEY    = "server-private-key";
    static constexpr const char* PUBLIC_KEY     = "server-public-key";

    class SESSION {
    public:
        static constexpr const char* RESOLVE    = "resolve";
        static constexpr const char* CONNECT    = "connect";
        static constexpr const char* SSL_HANDSHAKE  = "ssl_handshake";
        static constexpr const char* HANDSHAKE  = "handshake";

        static const int MAX_RETRY_COUNT;
        static const long RETRY_COUNT_DELAY;
    };

    // string constants
    static const char* PEER_INDEX;

    static const std::vector<std::string> DB_LIST;
};

}
}

#endif /* CONSTANTS_HPP */

