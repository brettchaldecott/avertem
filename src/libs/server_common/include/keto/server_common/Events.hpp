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
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    Events() = delete;
    Events(const Events& orig) = delete;
    virtual ~Events() = delete;

    // events for the password
    static const char* REQUEST_PASSWORD;

    // events for the key store
    static const char* REQUEST_SESSION_KEY;
    static const char* REMOVE_SESSION_KEY;
    
    // events for the router
    static const char* UPDATE_STATUS_ROUTE_MESSSAGE;
    static const char* ROUTE_MESSAGE;
    static const char* QUEUE_MESSAGE;
    static const char* PERSIST_BLOCK_MESSAGE;
    static const char* REGISTER_SERVICE_MESSAGE;
    static const char* REGISTER_RPC_PEER_CLIENT;
    static const char* REGISTER_RPC_PEER_SERVER;
    static const char* DEREGISTER_RPC_PEER;
    static const char* ACTIVATE_RPC_PEER;
    static const char* PUSH_RPC_PEER;
    static const char* RPC_CLIENT_ACTIVATE_RPC_PEER;
    static const char* RPC_SERVER_ACTIVATE_RPC_PEER;

    // get the chain tangle information
    static const char* GET_ACCOUNT_TANGLE;
    
    // account events
    static const char* CHECK_ACCOUNT_MESSAGE;
    static const char* GET_NODE_ACCOUNT_ROUTING;
    static const char* APPLY_ACCOUNT_DIRTY_TRANSACTION_MESSAGE;
    static const char* APPLY_ACCOUNT_TRANSACTION_MESSAGE;
    
    // rpc client route transaction
    static const char* RPC_CLIENT_TRANSACTION;
    static const char* RPC_CLIENT_REQUEST_BLOCK_SYNC;
    static const char* RPC_SERVER_REQUEST_BLOCK_SYNC;
    static const char* BLOCK_DB_REQUEST_BLOCK_SYNC;
    static const char* BLOCK_DB_RESPONSE_BLOCK_SYNC;
    static const char* BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY;
    static const char* IS_BLOCK_SYNC_COMPLETE;

    // rpc server route transaction
    static const char* RPC_SERVER_TRANSACTION;
    static const char* RPC_SERVER_BLOCK;
    static const char* RPC_CLIENT_BLOCK;

    // network state information
    static const char* REQUEST_NETWORK_STATE_CLIENT;
    static const char* REQUEST_NETWORK_STATE_SERVER;
    static const char* ACTIVATE_NETWORK_STATE_CLIENT;
    static const char* ACTIVATE_NETWORK_STATE_SERVER;
    static const char* UPDATE_NETWORK_STATE;

    // balancer
    static const char* BALANCER_MESSAGE;
    
    // balancer
    static const char* BLOCK_MESSAGE;
    static const char* BLOCK_PERSIST_MESSAGE;

    // block producer
    static const char* ENABLE_BLOCK_PRODUCER;
    
    // action messages
    static const char* EXECUTE_ACTION_MESSAGE;
    static const char* EXECUTE_HTTP_CONTRACT_MESSAGE;
    
    // rpc messages
    static const char* RPC_SEND_MESSAGE;
    
    // retrieve the contract
    static const char* GET_CONTRACT;

    // retrieve the contract
    static const char* CLEAR_DIRTY_CACHE;
    
    // sparql events
    static const char* SPARQL_QUERY_MESSAGE;
    static const char* DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE;
    static const char* SPARQL_QUERY_WITH_RESULTSET_MESSAGE;

    class ENCRYPT_ASN1 {
    public:
        static const char* ENCRYPT;
        static const char* DECRYPT;
    };

    class ENCRYPT_NETWORK_BYTES {
    public:
        static const char* ENCRYPT;
        static const char* DECRYPT;
    };

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
        static const char* MEMORY_VAULT_MANAGER;
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
        static const char* MEMORY_VAULT_MANAGER;
    };

    // consensus events
    class CONSENSUS_SESSION_ACCEPTED {
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
        static const char* MEMORY_VAULT_MANAGER;
    };


    class CONSENSUS_SESSION_CHECK {
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
        static const char* MEMORY_VAULT_MANAGER;
    };

    class CONSENSUS_HEARTBEAT {
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
        static const char* MEMORY_VAULT_MANAGER;
    };


    class CONSENSUS_SESSION_STATE {
    public:
        static const char* BLOCK;
        static const char* MEMORY_VAULT_MANAGER;
    };


    // memory vault methods
    class MEMORY_VAULT {
    public:
        static const char* CREATE_VAULT;
        static const char* ADD_ENTRY;
        static const char* SET_ENTRY;
        static const char* GET_ENTRY;
        static const char* REMOVE_ENTRY;

    };

    class NETWORK_FEE_INFO {
    public:
        static const char* GET_NETWORK_FEE;
        static const char* SET_NETWORK_FEE;
    };

    class BLOCK_PRODUCER_ELECTION {
    public:
        static const char* ELECT_RPC_CLIENT;
        static const char* ELECT_RPC_SERVER;
        static const char* ELECT_RPC_REQUEST;
        static const char* ELECT_RPC_RESPONSE;
        static const char* ELECT_RPC_PUBLISH_SERVER;
        static const char* ELECT_RPC_PUBLISH_CLIENT;
        static const char* ELECT_RPC_PROCESS_PUBLISH;
        static const char* ELECT_RPC_CONFIRMATION_SERVER;
        static const char* ELECT_RPC_CONFIRMATION_CLIENT;
        static const char* ELECT_RPC_PROCESS_CONFIRMATION;
    };

    class ROUTER_QUERY {
    public:
        static const char* ELECT_ROUTER_PEER;
        static const char* ELECT_RPC_PROCESS_PUBLISH;
        static const char* ELECT_RPC_PROCESS_CONFIRMATION;
        static const char* PUSH_RPC_PEER;
        static const char* PROCESS_PUSH_RPC_PEER;
        static const char* GET_PUBLISHED_ELECTION_INFO;
        static const char* SET_PUBLISHED_ELECTION_INFO;
    };

    class BLOCK_QUERY {
    public:
        static const char* GET_BLOCKS;
        static const char* GET_BLOCK;
        static const char* GET_BLOCK_TRANSACTIONS;
        static const char* GET_TRANSACTION;
        static const char* GET_ACCOUNT_TRANSACTIONS;
    };

    class PRODUCER_QUERY {
    public:
        static const char* GET_PRODUCER;
    };


    static const char* GET_SOFTWARE_CONSENSUS_MESSAGE;
    static const char* VALIDATE_SOFTWARE_CONSENSUS_MESSAGE;

    static const char* GET_NETWORK_SESSION_KEYS;
    static const char* SET_NETWORK_SESSION_KEYS;

    static const char* GET_MASTER_NETWORK_KEYS;
    static const char* SET_MASTER_NETWORK_KEYS;

    static const char* GET_NETWORK_KEYS;
    static const char* SET_NETWORK_KEYS;

    static const char* IS_MASTER;

    class PEER_TYPES {
    public:
        static const char* CLIENT;
        static const char* SERVER;
    };

private:

};


}
}


#endif /* EVENTS_HPP */

