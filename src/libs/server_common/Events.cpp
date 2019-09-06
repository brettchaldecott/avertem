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

// request the password
const char* Events::REQUEST_PASSWORD = "REQUEST_PASSWORD";

// events for the key store
const char* Events::REQUEST_SESSION_KEY = "REQUEST_SESSION_KEY";
const char* Events::REMOVE_SESSION_KEY = "REMOVE_SESSION_KEY";

// events for the router
const char* Events::UPDATE_STATUS_ROUTE_MESSSAGE  = "UPDATE_STATUS_ROUTE_MESSSAGE";
const char* Events::ROUTE_MESSAGE       = "ROUTE_MESSAGE";
const char* Events::PERSIST_BLOCK_MESSAGE = "PERSIST_BLOCK_MESSAGE";
const char* Events::REGISTER_SERVICE_MESSAGE = "REGISTER_SERVICE_MESSAGE";
const char* Events::REGISTER_RPC_PEER   = "REGISTER_RPC_PEER";
const char* Events::DEREGISTER_RPC_PEER = "DEREGISTER_RPC_PEER";
const char* Events::ACTIVATE_RPC_PEER   = "ACTIVATE_RPC_PEER";
const char* Events::PUSH_RPC_PEER       = "PUSH_RPC_PEER";
const char* Events::RPC_CLIENT_ACTIVATE_RPC_PEER = "RPC_CLIENT_ACTIVATE_RPC_PEER";
const char* Events::RPC_SERVER_ACTIVATE_RPC_PEER = "RPC_SERVER_ACTIVATE_RPC_PEER";

// get account tangle
const char* Events::GET_ACCOUNT_TANGLE = "GET_ACCOUNT_TANGLE";

// account events
const char* Events::CHECK_ACCOUNT_MESSAGE = "CHECK_ACCOUNT_MESSAGE";
const char* Events::GET_NODE_ACCOUNT_ROUTING = "GET_NODE_ACCOUNT_ROUTING";
const char* Events::APPLY_ACCOUNT_DIRTY_TRANSACTION_MESSAGE = "APPLY_ACCOUNT_DIRTY_TRANSACTION_MESSAGE";
const char* Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE = "APPLY_ACCOUNT_TRANSACTION_MESSAGE";

// rpc client route transaction
const char* Events::RPC_CLIENT_TRANSACTION = "RPC_CLIENT_TRANSACTION";
const char* Events::RPC_CLIENT_REQUEST_BLOCK_SYNC = "RPC_CLIENT_REQUEST_BLOCK_SYNC";
const char* Events::BLOCK_DB_REQUEST_BLOCK_SYNC = "BLOCK_DB_REQUEST_BLOCK_SYNC";
const char* Events::BLOCK_DB_RESPONSE_BLOCK_SYNC = "BLOCK_DB_RESPONSE_BLOCK_SYNC";
const char* Events::BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY = "BLOCK_DB_REQUEST_BLOCK_SYNC_RETRY";

// rpc server route transaction
const char* Events::RPC_SERVER_TRANSACTION = "RPC_SERVER_TRANSACTION";
const char* Events::RPC_SERVER_BLOCK       = "RPC_SERVER_BLOCK";
const char* Events::RPC_CLIENT_BLOCK       = "RPC_CLIENT_BLOCK";

// balancer
const char* Events::BALANCER_MESSAGE    = "BALANCER_MESSAGE";

// block message
const char* Events::BLOCK_MESSAGE       = "BLOCK_MESSAGE";
const char* Events::BLOCK_PERSIST_MESSAGE = "BLOCK_PERSIST_MESSAGE";

const char* Events::ENABLE_BLOCK_PRODUCER = "ENABLE_BLOCK_PRODUCER";

// block message
const char* Events::EXECUTE_ACTION_MESSAGE  = "EXECUTE_ACTION_MESSAGE";
const char* Events::EXECUTE_HTTP_CONTRACT_MESSAGE = "EXECUTE_HTTP_CONTRACT_MESSAGE";

// rpc messages
const char* Events::RPC_SEND_MESSAGE    = "RPC_SEND_MESSAGE";

// request the contract by name or hash
const char* Events::GET_CONTRACT  = "GET_CONTRACT";

// clear dirty cache
const char* Events::CLEAR_DIRTY_CACHE = "CLEAR_DIRTY_CACHE";

// sparql events
const char* Events::SPARQL_QUERY_MESSAGE                        = "SPARQL_QUERY_MESSAGE";
const char* Events::DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE   = "DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE";
const char* Events::SPARQL_QUERY_WITH_RESULTSET_MESSAGE         = "SPARQL_QUERY_WITH_RESULTSET_MESSAGE";


const char* Events::ENCRYPT_ASN1::ENCRYPT                       = "SIGNED_BLOCK_ENCRYPT";
const char* Events::ENCRYPT_ASN1::DECRYPT                       = "SIGNED_BLOCK_DECRYPT";

// network encryption events
const char* Events::ENCRYPT_NETWORK_BYTES::ENCRYPT              = "ENCRYPT_NETWORK_BYTES";
const char* Events::ENCRYPT_NETWORK_BYTES::DECRYPT              = "DECRYPT_NETWORK_BYTES";

// the test consensus method cannot be called as it is loaded before the event
// registry
const char* Events::CONSENSUS::TEST         = "CONSENSUS_TEST";


const char* Events::CONSENSUS::TRANSACTION  = "CONSENSUS_TRANSACTION";
const char* Events::CONSENSUS::ROUTER       = "CONSENSUS_ROUTER";
const char* Events::CONSENSUS::HTTPD        = "CONSENSUS_HTTPD";
const char* Events::CONSENSUS::EVENT        = "CONSENSUS_EVENT";
const char* Events::CONSENSUS::BLOCK        = "CONSENSUS_BLOCK";
const char* Events::CONSENSUS::SANDBOX      = "CONSENSUS_SANDBOX";
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

const char* Events::CONSENSUS_SESSION_CHECK::TEST         = "CONSENSUS_SESSION_CHECK_TEST";
const char* Events::CONSENSUS_SESSION_CHECK::TRANSACTION  = "CONSENSUS_SESSION_CHECK_TRANSACTION";
const char* Events::CONSENSUS_SESSION_CHECK::ROUTER       = "CONSENSUS_SESSION_CHECK_ROUTER";
const char* Events::CONSENSUS_SESSION_CHECK::HTTPD        = "CONSENSUS_SESSION_CHECK_HTTPD";
const char* Events::CONSENSUS_SESSION_CHECK::EVENT        = "CONSENSUS_SESSION_CHECK_EVENT";
const char* Events::CONSENSUS_SESSION_CHECK::BLOCK        = "CONSENSUS_SESSION_CHECK_BLOCK";
const char* Events::CONSENSUS_SESSION_CHECK::SANDBOX      = "CONSENSUS_SESSION_CHECK_SANDBOX";
const char* Events::CONSENSUS_SESSION_CHECK::VERSION      = "CONSENSUS_SESSION_CHECK_VERSION";
const char* Events::CONSENSUS_SESSION_CHECK::KEYSTORE     = "CONSENSUS_SESSION_CHECK_KEY_STORE";
const char* Events::CONSENSUS_SESSION_CHECK::BALANCER     = "CONSENSUS_SESSION_CHECK_BALANCER";
const char* Events::CONSENSUS_SESSION_CHECK::ACCOUNT      = "CONSENSUS_SESSION_CHECK_ACCOUNT";
const char* Events::CONSENSUS_SESSION_CHECK::RPC_CLIENT   = "CONSENSUS_SESSION_CHECK_RPC_CLIENT";
const char* Events::CONSENSUS_SESSION_CHECK::RPC_SERVER   = "CONSENSUS_SESSION_CHECK_RPC_SERVER";
const char* Events::CONSENSUS_SESSION_CHECK::CONSENSUS_QUERY    = "CONSENSUS_SESSION_CHECK_CONSENSUS";
const char* Events::CONSENSUS_SESSION_CHECK::MEMORY_VAULT_MANAGER    = "CONSENSUS_SESSION_CHECK_MEMORY_VAULT_MANAGER";

const char* Events::CONSENSUS_HEARTBEAT::TEST         = "CONSENSUS_HEARTBEAT_TEST";
const char* Events::CONSENSUS_HEARTBEAT::TRANSACTION  = "CONSENSUS_HEARTBEAT_TRANSACTION";
const char* Events::CONSENSUS_HEARTBEAT::ROUTER       = "CONSENSUS_HEARTBEAT_ROUTER";
const char* Events::CONSENSUS_HEARTBEAT::HTTPD        = "CONSENSUS_HEARTBEAT_HTTPD";
const char* Events::CONSENSUS_HEARTBEAT::EVENT        = "CONSENSUS_HEARTBEAT_EVENT";
const char* Events::CONSENSUS_HEARTBEAT::BLOCK        = "CONSENSUS_HEARTBEAT_BLOCK";
const char* Events::CONSENSUS_HEARTBEAT::SANDBOX      = "CONSENSUS_HEARTBEAT_SANDBOX";
const char* Events::CONSENSUS_HEARTBEAT::VERSION      = "CONSENSUS_HEARTBEAT_VERSION";
const char* Events::CONSENSUS_HEARTBEAT::KEYSTORE     = "CONSENSUS_HEARTBEAT_KEY_STORE";
const char* Events::CONSENSUS_HEARTBEAT::BALANCER     = "CONSENSUS_HEARTBEAT_BALANCER";
const char* Events::CONSENSUS_HEARTBEAT::ACCOUNT      = "CONSENSUS_HEARTBEAT_ACCOUNT";
const char* Events::CONSENSUS_HEARTBEAT::RPC_CLIENT   = "CONSENSUS_HEARTBEAT_RPC_CLIENT";
const char* Events::CONSENSUS_HEARTBEAT::RPC_SERVER   = "CONSENSUS_HEARTBEAT_RPC_SERVER";
const char* Events::CONSENSUS_HEARTBEAT::CONSENSUS_QUERY    = "CONSENSUS_HEARTBEAT_CONSENSUS";
const char* Events::CONSENSUS_HEARTBEAT::MEMORY_VAULT_MANAGER    = "CONSENSUS_HEARTBEAT_MEMORY_VAULT_MANAGER";

const char* Events::CONSENSUS_SESSION_STATE::BLOCK                  = "CONSENSUS_SESSION_STATE_BLOCK";
const char* Events::CONSENSUS_SESSION_STATE::MEMORY_VAULT_MANAGER   = "CONSENSUS_SESSION_STATE_MEMORY_VAULT_MANAGER";

const char* Events::MEMORY_VAULT::CREATE_VAULT  = "MEMORY_VAULT::CREATE_VAULT";
const char* Events::MEMORY_VAULT::ADD_ENTRY     = "MEMORY_VAULT::ADD_ENTRY";
const char* Events::MEMORY_VAULT::GET_ENTRY     = "MEMORY_VAULT::GET_ENTRY";
const char* Events::MEMORY_VAULT::SET_ENTRY     = "MEMORY_VAULT::SET_ENTRY";
const char* Events::MEMORY_VAULT::REMOVE_ENTRY  = "MEMORY_VAULT::REMOVE_ENTRY";

const char* Events::NETWORK_FEE_INFO::GET_NETWORK_FEE   ="NETWORK_FEE_INFO::GET_NETWORK_FEE";
const char* Events::NETWORK_FEE_INFO::SET_NETWORK_FEE   ="NETWORK_FEE_INFO::SET_NETWORK_FEE";

const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CLIENT           = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_CLIENT";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_SERVER           = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_SERVER";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_REQUEST          = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_REQUEST";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE         = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_RESPONSE";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_SERVER   = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_SERVER";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_CLIENT   = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_CLIENT";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_PUBLISH  = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_PUBLISH";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_SERVER      = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_SERVER";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_CLIENT      = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_CLIENT";
const char* Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_CONFIRMATION     = "BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_CONFIRMATION";

const char* Events::ROUTER_QUERY::ELECT_ROUTER_PEER                     = "ROUTER_QUERY::ELECT_ROUTER_PEER";
const char* Events::ROUTER_QUERY::ELECT_RPC_PROCESS_PUBLISH             = "ROUTER_QUERY::ELECT_RPC_PROCESS_PUBLISH";
const char* Events::ROUTER_QUERY::ELECT_RPC_PROCESS_CONFIRMATION        = "ROUTER_QUERY::ELECT_RPC_PROCESS_CONFIRMATION";
const char* Events::ROUTER_QUERY::PUSH_RPC_PEER                         = "ROUTER_QUERY::PUSH_RPC_PEER";
const char* Events::ROUTER_QUERY::PROCESS_PUSH_RPC_PEER                 = "ROUTER_QUERY::PROCESS_PUSH_RPC_PEER";


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

const char* Events::IS_MASTER               = "IS_MASTER";


const char* Events::PEER_TYPES::CLIENT      = "CLIENT";
const char* Events::PEER_TYPES::SERVER      = "SERVER";

}
}