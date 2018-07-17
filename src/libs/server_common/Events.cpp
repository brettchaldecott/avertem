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


// events for the key store
const char* Events::REQUEST_SESSION_KEY = "REQUEST_SESSION_KEY";
const char* Events::REMOVE_SESSION_KEY = "REMOVE_SESSION_KEY";

// events for the router
const char* Events::UPDATE_STATUS_ROUTE_MESSSAGE  = "UPDATE_STATUS_ROUTE_MESSSAGE";
const char* Events::ROUTE_MESSAGE = "ROUTE_MESSAGE";
const char* Events::REGISTER_SERVICE_MESSAGE = "REGISTER_SERVICE_MESSAGE";

// account events
const char* Events::CHECK_ACCOUNT_MESSAGE = "CHECK_ACCOUNT_MESSAGE";
const char* Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE = "APPLY_ACCOUNT_TRANSACTION_MESSAGE";

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
const char* Events::CONSENSUS::SANBOX       = "CONSENSUS_SANDBOX";
const char* Events::CONSENSUS::VERSION      = "CONSENSUS_VERSION";
const char* Events::CONSENSUS::KEYSTORE     = "CONSENSUS_KEY_STORE";
const char* Events::CONSENSUS::BALANCER     = "CONSENSUS_BALANCER";
const char* Events::CONSENSUS::ACCOUNT      = "CONSENSUS_ACCOUNT";
const char* Events::CONSENSUS::RPC_CLIENT   = "CONSENSUS_RPC_CLIENT";
const char* Events::CONSENSUS::RPC_SERVER   = "CONSENSUS_RPC_SERVER";


const char* Events::GET_SOFTWARE_CONSENSUS_MESSAGE
                                            = "GET_SOFTWARE_CONSENSUS_MESSAGE";

}
}