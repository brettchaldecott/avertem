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
    return OBFUSCATED("$Id:$");
}


const char* Constants::PUBLIC_KEY_DIR = "public-key-dir";
const char* Constants::ACCOUNT_HASH = "account-hash";

// keys for server
const char* Constants::PRIVATE_KEY    = "server-private-key";
const char* Constants::PUBLIC_KEY     = "server-public-key";


const char* Constants::SERVICE::ROUTE       = "route";
const char* Constants::SERVICE::BALANCE     = "balance";
const char* Constants::SERVICE::BLOCK       = "block";
const char* Constants::SERVICE::PROCESS     = "process";


const char* Constants::CONTRACTS::BASE_ACCOUNT_CONTRACT     = "base_account_transaction";

const char* Constants::ACCOUNT_ACTIONS::DEBIT     = "debit";
const char* Constants::ACCOUNT_ACTIONS::CREDIT     = "credit";


const char* Constants::RPC_COMMANDS::HELLO = "HELLO";
const char* Constants::RPC_COMMANDS::HELLO_CONSENSUS = "HELLO_CONSENSUS";
const char* Constants::RPC_COMMANDS::PEERS = "PEERS";
const char* Constants::RPC_COMMANDS::TRANSACTION = "TRANSACTION";
const char* Constants::RPC_COMMANDS::CONSENSUS_SESSION = "CONSENSUS_SESSION";
const char* Constants::RPC_COMMANDS::CONSENSUS = "CONSENSUS";
const char* Constants::RPC_COMMANDS::ROUTE = "ROUTE";
const char* Constants::RPC_COMMANDS::ROUTE_UPDATE = "ROUTE_UPDATE";
const char* Constants::RPC_COMMANDS::SERVICES = "SERVICES";
const char* Constants::RPC_COMMANDS::CLOSE = "CLOSE";
const char* Constants::RPC_COMMANDS::GO_AWAY = "GO_AWAY";
const char* Constants::RPC_COMMANDS::ACCEPTED = "ACCEPTED";

}
}