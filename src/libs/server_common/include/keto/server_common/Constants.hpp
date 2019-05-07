/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on February 16, 2018, 8:18 AM
 */

#ifndef SERVER_COMMON_CONSTANTS_HPP
#define SERVER_COMMON_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace server_common {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    static const char* PUBLIC_KEY_DIR;
    static const char* ACCOUNT_HASH;
    static const char* FEE_ACCOUNT_HASH;
    
    // keys for server
    static const char* PRIVATE_KEY;
    static const char* PUBLIC_KEY;

    
    class SERVICE {
    public:
        static const char* ROUTE;
        static const char* BALANCE;
        static const char* BLOCK;
        static const char* PROCESS;
    };
    
    class CONTRACTS {
    public:
        static const char* BASE_ACCOUNT_CONTRACT;
        static const char* NESTED_TRANSACTION_CONTRACT;
        static const char* FEE_PAYMENT_CONTRACT;
        
    };
    
    class ACCOUNT_ACTIONS {
    public:
        static const char* DEBIT;
        static const char* CREDIT;
    };
    
    class RPC_COMMANDS {
    public:
        static const char* HELLO;
        static const char* HELLO_CONSENSUS;
        static const char* PEERS;
        static const char* REGISTER;
        static const char* TRANSACTION;
        static const char* TRANSACTION_PROCESSED;
        static const char* BLOCK;
        static const char* BLOCK_PROCESSED;
        static const char* BLOCK_REQUEST;
        static const char* CONSENSUS_SESSION;
        static const char* CONSENSUS;
        static const char* ROUTE;
        static const char* ROUTE_UPDATE;
        static const char* SERVICES;
        static const char* CLOSE;
        static const char* GO_AWAY;
        static const char* ACCEPTED;
        static const char* REQUEST_NETWORK_SESSION_KEYS;
        static const char* RESPONSE_NETWORK_SESSION_KEYS;
        static const char* REQUEST_MASTER_NETWORK_KEYS;
        static const char* RESPONSE_MASTER_NETWORK_KEYS;
        static const char* REQUEST_NETWORK_KEYS;
        static const char* RESPONSE_NETWORK_KEYS;
        static const char* REQUEST_NETWORK_FEES;
        static const char* RESPONSE_NETWORK_FEES;
        static const char* RESPONSE_RETRY;
    };
    
private:

};


}
}

#endif /* CONSTANTS_HPP */

