/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Events.cpp
 * Author: ubuntu
 * 
 * Created on February 17, 2018, 12:45 PM
 */

#include "keto/server_common/Events.hpp"

namespace keto {
namespace server_common {

std::string Events::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


// events for the key store
const char* Events::REQUEST_SESSION_KEY = "REQUEST_SESSION_KEY";
const char* Events::REMOVE_SESSION_KEY = "REMOVE_SESSION_KEY";

// events for the router
const char* Events::UPDATE_STATUS_ROUTE_MESSSAGE  = "UPDATE_STATUS_ROUTE_MESSSAGE";
const char* Events::ROUTE_MESSAGE = "ROUTE_MESSAGE";
const char* Events::REGISTER_SERVICE_MESSAGE = "REGISTER_SERVICE_MESSAGE";
const char* Events::REGISTER_RPC_PEER = "REGISTER_RPC_PEER";

// account events
const char* Events::CHECK_ACCOUNT_MESSAGE = "CHECK_ACCOUNT_MESSAGE";
const char* Events::GET_NODE_ACCOUNT_ROUTING = "GET_NODE_ACCOUNT_ROUTING";
const char* Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE = "APPLY_ACCOUNT_TRANSACTION_MESSAGE";

// rpc client route transaction
const char* Events::RPC_CLIENT_TRANSACTION = "RPC_CLIENT_TRANSACTION";

// rpc server route transaction
const char* Events::RPC_SERVER_TRANSACTION = "RPC_SERVER_TRANSACTION";

// balancer
const char* Events::BALANCER_MESSAGE    = "BALANCER_MESSAGE";

// block message
const char* Events::BLOCK_MESSAGE       = "BLOCK_MESSAGE";

// block message
const char* Events::EXECUTE_ACTION_MESSAGE  = "EXECUTE_ACTION_MESSAGE";

// rpc messages
const char* Events::RPC_SEND_MESSAGE    = "RPC_SEND_MESSAGE";

// request the contract by name or hash
const char* Events::GET_CONTRACT  = "GET_CONTRACT";

// sparql events
const char* Events::SPARQL_QUERY_MESSAGE = "SPARQL_QUERY_MESSAGE";

// the test consensus method cannot be called as it is loaded before the event
// registry
const char* Events::CONSENSUS::TEST         = "CONSENSUS_TEST";


const char* Events::CONSENSUS::TRANSACTION  = "CONSENSUS_TRANSACTION";
const char* Events::CONSENSUS::ROUTER       = "CONSENSUS_ROUTER";
const char* Events::CONSENSUS::HTTPD        = "CONSENSUS_HTTPD";
const char* Events::CONSENSUS::EVENT        = "CONSENSUS_EVENT";
const char* Events::CONSENSUS::BLOCK        = "CONSENSUS_BLOCK";
const char* Events::CONSENSUS::SANDBOX       = "CONSENSUS_SANDBOX";
const char* Events::CONSENSUS::VERSION      = "CONSENSUS_VERSION";
const char* Events::CONSENSUS::KEYSTORE     = "CONSENSUS_KEY_STORE";
const char* Events::CONSENSUS::BALANCER     = "CONSENSUS_BALANCER";
const char* Events::CONSENSUS::ACCOUNT      = "CONSENSUS_ACCOUNT";
const char* Events::CONSENSUS::RPC_CLIENT   = "CONSENSUS_RPC_CLIENT";
const char* Events::CONSENSUS::RPC_SERVER   = "CONSENSUS_RPC_SERVER";
const char* Events::CONSENSUS::CONSENSUS_QUERY    = "CONSENSUS_CONSENSUS";
const char* Events::CONSENSUS::MEMORY_VAULT_MANAGER    = "CONSENSUS_MEMORY_VAULT_MANAGER";

const char* Events::CONSENSUS_SESSION::TEST         = "CONSENSUS_SESSION_TEST";
const char* Events::CONSENSUS_SESSION::TRANSACTION  = "CONSENSUS_SESSION_TRANSACTION";
const char* Events::CONSENSUS_SESSION::ROUTER       = "CONSENSUS_SESSION_ROUTER";
const char* Events::CONSENSUS_SESSION::HTTPD        = "CONSENSUS_SESSION_HTTPD";
const char* Events::CONSENSUS_SESSION::EVENT        = "CONSENSUS_SESSION_EVENT";
const char* Events::CONSENSUS_SESSION::BLOCK        = "CONSENSUS_SESSION_BLOCK";
const char* Events::CONSENSUS_SESSION::SANDBOX       = "CONSENSUS_SESSION_SANDBOX";
const char* Events::CONSENSUS_SESSION::VERSION      = "CONSENSUS_SESSION_VERSION";
const char* Events::CONSENSUS_SESSION::KEYSTORE     = "CONSENSUS_SESSION_KEY_STORE";
const char* Events::CONSENSUS_SESSION::BALANCER     = "CONSENSUS_SESSION_BALANCER";
const char* Events::CONSENSUS_SESSION::ACCOUNT      = "CONSENSUS_SESSION_ACCOUNT";
const char* Events::CONSENSUS_SESSION::RPC_CLIENT   = "CONSENSUS_SESSION_RPC_CLIENT";
const char* Events::CONSENSUS_SESSION::RPC_SERVER   = "CONSENSUS_SESSION_RPC_SERVER";
const char* Events::CONSENSUS_SESSION::CONSENSUS_QUERY    = "CONSENSUS_SESSION_CONSENSUS";
const char* Events::CONSENSUS_SESSION::MEMORY_VAULT_MANAGER    = "CONSENSUS_SESSION_MEMORY_VAULT_MANAGER";


const char* Events::CONSENSUS_SESSION_ACCEPTED::TEST         = "CONSENSUS_SESSION_ACCEPTED_TEST";
const char* Events::CONSENSUS_SESSION_ACCEPTED::TRANSACTION  = "CONSENSUS_SESSION_ACCEPTED_TRANSACTION";
const char* Events::CONSENSUS_SESSION_ACCEPTED::ROUTER       = "CONSENSUS_SESSION_ACCEPTED_ROUTER";
const char* Events::CONSENSUS_SESSION_ACCEPTED::HTTPD        = "CONSENSUS_SESSION_ACCEPTED_HTTPD";
const char* Events::CONSENSUS_SESSION_ACCEPTED::EVENT        = "CONSENSUS_SESSION_ACCEPTED_EVENT";
const char* Events::CONSENSUS_SESSION_ACCEPTED::BLOCK        = "CONSENSUS_SESSION_ACCEPTED_BLOCK";
const char* Events::CONSENSUS_SESSION_ACCEPTED::SANDBOX      = "CONSENSUS_SESSION_ACCEPTED_SANDBOX";
const char* Events::CONSENSUS_SESSION_ACCEPTED::VERSION      = "CONSENSUS_SESSION_ACCEPTED_VERSION";
const char* Events::CONSENSUS_SESSION_ACCEPTED::KEYSTORE     = "CONSENSUS_SESSION_ACCEPTED_KEY_STORE";
const char* Events::CONSENSUS_SESSION_ACCEPTED::BALANCER     = "CONSENSUS_SESSION_ACCEPTED_BALANCER";
const char* Events::CONSENSUS_SESSION_ACCEPTED::ACCOUNT      = "CONSENSUS_SESSION_ACCEPTED_ACCOUNT";
const char* Events::CONSENSUS_SESSION_ACCEPTED::RPC_CLIENT   = "CONSENSUS_SESSION_ACCEPTED_RPC_CLIENT";
const char* Events::CONSENSUS_SESSION_ACCEPTED::RPC_SERVER   = "CONSENSUS_SESSION_ACCEPTED_RPC_SERVER";
const char* Events::CONSENSUS_SESSION_ACCEPTED::CONSENSUS_QUERY    = "CONSENSUS_SESSION_ACCEPTED_CONSENSUS";
const char* Events::CONSENSUS_SESSION_ACCEPTED::MEMORY_VAULT_MANAGER    = "CONSENSUS_SESSION_ACCEPTED_MEMORY_VAULT_MANAGER";

const char* Events::CONSENSUS_SESSION_STATE::BLOCK                  = "CONSENSUS_SESSION_STATE_BLOCK";
const char* Events::CONSENSUS_SESSION_STATE::MEMORY_VAULT_MANAGER   = "CONSENSUS_SESSION_STATE_MEMORY_VAULT_MANAGER";

const char* Events::MEMORY_VAULT::CREATE_VAULT  = "CREATE_VAULT";
const char* Events::MEMORY_VAULT::ADD_ENTRY     = "ADD_ENTRY";
const char* Events::MEMORY_VAULT::GET_ENTRY     = "GET_ENTRY";
const char* Events::MEMORY_VAULT::SET_ENTRY     = "SET_ENTRY";
const char* Events::MEMORY_VAULT::REMOVE_ENTRY  = "REMOVE_ENTRY";


const char* Events::GET_SOFTWARE_CONSENSUS_MESSAGE
                                            = "GET_SOFTWARE_CONSENSUS_MESSAGE";
const char* Events::VALIDATE_SOFTWARE_CONSENSUS_MESSAGE
                                            = "VALDATE_SOFTWARE_CONSENSUS_MESSAGE";

const char* Events::GET_NETWORK_SESSION_KEYS = "GET_NETWORK_SESSION_KEYS";
const char* Events::SET_NETWORK_SESSION_KEYS = "SET_NETWORK_SESSION_KEYS";

const char* Events::GET_MASTER_NETWORK_KEYS = "GET_MASTER_NETWORK_KEYS";
const char* Events::SET_MASTER_NETWORK_KEYS = "SET_MASTER_NETWORK_KEYS";

const char* Events::GET_NETWORK_KEYS        = "GET_NETWORK_KEYS";
const char* Events::SET_NETWORK_KEYS        = "SET_NETWORK_KEYS";

}
}