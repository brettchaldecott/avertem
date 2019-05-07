/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.cpp
 * Author: ubuntu
 * 
 * Created on February 16, 2018, 8:18 AM
 */

#include "keto/server_common/Constants.hpp"


namespace keto {
namespace server_common {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


const char* Constants::PUBLIC_KEY_DIR = "public-key-dir";
const char* Constants::ACCOUNT_HASH = "account-hash";
const char* Constants::FEE_ACCOUNT_HASH = "fee-account-hash";

// keys for server
const char* Constants::PRIVATE_KEY    = "server-private-key";
const char* Constants::PUBLIC_KEY     = "server-public-key";


const char* Constants::SERVICE::ROUTE       = "route";
const char* Constants::SERVICE::BALANCE     = "balance";
const char* Constants::SERVICE::BLOCK       = "block";
const char* Constants::SERVICE::PROCESS     = "process";


const char* Constants::CONTRACTS::BASE_ACCOUNT_CONTRACT     =   "base_account_transaction";
const char* Constants::CONTRACTS::NESTED_TRANSACTION_CONTRACT = "nested_transaction";
const char* Constants::CONTRACTS::FEE_PAYMENT_CONTRACT =        "fee_payment";

const char* Constants::ACCOUNT_ACTIONS::DEBIT     = "debit";
const char* Constants::ACCOUNT_ACTIONS::CREDIT     = "credit";


const char* Constants::RPC_COMMANDS::HELLO = "HELLO";
const char* Constants::RPC_COMMANDS::HELLO_CONSENSUS = "HELLO_CONSENSUS";
const char* Constants::RPC_COMMANDS::PEERS = "PEERS";
const char* Constants::RPC_COMMANDS::REGISTER = "REGISTER";
const char* Constants::RPC_COMMANDS::TRANSACTION = "TRANSACTION";
const char* Constants::RPC_COMMANDS::TRANSACTION_PROCESSED = "TRANSACTION_PROCESSED";
const char* Constants::RPC_COMMANDS::BLOCK = "BLOCK";
const char* Constants::RPC_COMMANDS::BLOCK_PROCESSED = "BLOCK_PROCESSED";
const char* Constants::RPC_COMMANDS::BLOCK_REQUEST = "BLOCK_REQUEST";
const char* Constants::RPC_COMMANDS::CONSENSUS_SESSION = "CONSENSUS_SESSION";
const char* Constants::RPC_COMMANDS::CONSENSUS = "CONSENSUS";
const char* Constants::RPC_COMMANDS::ROUTE = "ROUTE";
const char* Constants::RPC_COMMANDS::ROUTE_UPDATE = "ROUTE_UPDATE";
const char* Constants::RPC_COMMANDS::SERVICES = "SERVICES";
const char* Constants::RPC_COMMANDS::CLOSE = "CLOSE";
const char* Constants::RPC_COMMANDS::GO_AWAY = "GO_AWAY";
const char* Constants::RPC_COMMANDS::ACCEPTED = "ACCEPTED";
const char* Constants::RPC_COMMANDS::REQUEST_NETWORK_SESSION_KEYS = "REQUEST_NETWORK_SESSION_KEYS";
const char* Constants::RPC_COMMANDS::RESPONSE_NETWORK_SESSION_KEYS = "RESPONSE_NETWORK_SESSION_KEYS";
const char* Constants::RPC_COMMANDS::REQUEST_MASTER_NETWORK_KEYS = "REQUEST_MASTER_NETWORK_KEYS";
const char* Constants::RPC_COMMANDS::RESPONSE_MASTER_NETWORK_KEYS = "RESPONSE_MASTER_NETWORK_KEYS";
const char* Constants::RPC_COMMANDS::REQUEST_NETWORK_KEYS = "REQUEST_NETWORK_KEYS";
const char* Constants::RPC_COMMANDS::RESPONSE_NETWORK_KEYS = "RESPONSE_NETWORK_KEYS";
const char* Constants::RPC_COMMANDS::REQUEST_NETWORK_FEES = "REQUEST_NETWORK_FEES";
const char* Constants::RPC_COMMANDS::RESPONSE_NETWORK_FEES = "RESPONSE_NETWORK_FEES";
const char* Constants::RPC_COMMANDS::RESPONSE_RETRY = "RESPONSE_RETRY";

}
}