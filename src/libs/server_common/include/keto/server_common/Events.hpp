/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Events.hpp
 * Author: ubuntu
 *
 * Created on February 17, 2018, 12:45 PM
 */

#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace server_common {

class Events {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();

    Events() = delete;
    Events(const Events& orig) = delete;
    virtual ~Events() = delete;
    
    // events for the key store
    static const char* REQUEST_SESSION_KEY;
    static const char* REMOVE_SESSION_KEY;
    
    // events for the router
    static const char* UPDATE_STATUS_ROUTE_MESSSAGE;
    static const char* ROUTE_MESSAGE;
    static const char* REGISTER_SERVICE_MESSAGE;
    
    // account events
    static const char* CHECK_ACCOUNT_MESSAGE;
    static const char* APPLY_ACCOUNT_TRANSACTION_MESSAGE;
    
    // balancer
    static const char* BALANCER_MESSAGE;
    
    // balancer
    static const char* BLOCK_MESSAGE;
    
    // action messages
    static const char* EXECUTE_ACTION_MESSAGE;
    
    // rpc messages
    static const char* RPC_SEND_MESSAGE;
    
    // retrieve the contract
    static const char* GET_CONTRACT;
    
    // sparql events
    static const char* SPARQL_QUERY_MESSAGE;
    
    // consensus events
    class CONSENSUS {
    public:
        static const char* TEST;
        static const char* TRANSACTION;
        static const char* ROUTER;
        static const char* HTTPD;
        static const char* EVENT;
        static const char* BLOCK;
        static const char* SANDBOX;
        static const char* VERSION;
        static const char* KEYSTORE;
        static const char* BALANCER;
        static const char* ACCOUNT;
        static const char* RPC_CLIENT;
        static const char* RPC_SERVER;
        static const char* CONSENSUS_QUERY;
    };
    
    // consensus events
    class CONSENSUS_SESSION {
    public:
        static const char* TEST;
        static const char* TRANSACTION;
        static const char* ROUTER;
        static const char* HTTPD;
        static const char* EVENT;
        static const char* BLOCK;
        static const char* SANDBOX;
        static const char* VERSION;
        static const char* KEYSTORE;
        static const char* BALANCER;
        static const char* ACCOUNT;
        static const char* RPC_CLIENT;
        static const char* RPC_SERVER;
        static const char* CONSENSUS_QUERY;
    };
    
    static const char* GET_SOFTWARE_CONSENSUS_MESSAGE;
    static const char* VALIDATE_SOFTWARE_CONSENSUS_MESSAGE;
    
private:

};


}
}


#endif /* EVENTS_HPP */

