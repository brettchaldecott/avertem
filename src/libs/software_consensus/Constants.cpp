/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.cpp
 * Author: ubuntu
 * 
 * Created on May 29, 2018, 11:13 AM
 */


#include "keto/server_common/Events.hpp"
#include "keto/software_consensus/Constants.hpp"


namespace keto {
namespace software_consensus {

const std::vector<std::string> Constants::EVENT_ORDER = {
    keto::server_common::Events::CONSENSUS::TEST,
    keto::server_common::Events::CONSENSUS::TRANSACTION,
    keto::server_common::Events::CONSENSUS::ROUTER,
    keto::server_common::Events::CONSENSUS::HTTPD,
    keto::server_common::Events::CONSENSUS::EVENT,
    keto::server_common::Events::CONSENSUS::BLOCK,
    keto::server_common::Events::CONSENSUS::SANDBOX,
    keto::server_common::Events::CONSENSUS::VERSION,
    keto::server_common::Events::CONSENSUS::KEYSTORE,
    keto::server_common::Events::CONSENSUS::BALANCER,
    keto::server_common::Events::CONSENSUS::ACCOUNT,
    keto::server_common::Events::CONSENSUS::RPC_CLIENT,
    keto::server_common::Events::CONSENSUS::RPC_SERVER,
    keto::server_common::Events::CONSENSUS::CONSENSUS_QUERY,
    keto::server_common::Events::CONSENSUS::MEMORY_VAULT_MANAGER};

const std::vector<std::string> Constants::CONSENSUS_SESSION_ORDER = {
    keto::server_common::Events::CONSENSUS_SESSION::BLOCK,
    keto::server_common::Events::CONSENSUS_SESSION::RPC_SERVER,
    keto::server_common::Events::CONSENSUS_SESSION::RPC_CLIENT,
    keto::server_common::Events::CONSENSUS_SESSION::TEST,
    keto::server_common::Events::CONSENSUS_SESSION::TRANSACTION,
    keto::server_common::Events::CONSENSUS_SESSION::ROUTER,
    keto::server_common::Events::CONSENSUS_SESSION::HTTPD,
    keto::server_common::Events::CONSENSUS_SESSION::EVENT,
    keto::server_common::Events::CONSENSUS_SESSION::SANDBOX,
    keto::server_common::Events::CONSENSUS_SESSION::VERSION,
    keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE,
    keto::server_common::Events::CONSENSUS_SESSION::BALANCER,
    keto::server_common::Events::CONSENSUS_SESSION::ACCOUNT,
    keto::server_common::Events::CONSENSUS_SESSION::CONSENSUS_QUERY,
    keto::server_common::Events::CONSENSUS_SESSION::MEMORY_VAULT_MANAGER};

const std::vector<std::string> Constants::CONSENSUS_SESSION_ACCEPTED = {
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::TEST,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::TRANSACTION,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::ROUTER,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::HTTPD,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::EVENT,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::SANDBOX,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::VERSION,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::KEYSTORE,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::BALANCER,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::ACCOUNT,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::RPC_CLIENT,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::RPC_SERVER,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::CONSENSUS_QUERY,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::MEMORY_VAULT_MANAGER,
    keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::BLOCK};

const std::vector<std::string> Constants::CONSENSUS_SESSION_CHECK = {
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::TEST,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::TRANSACTION,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::ROUTER,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::HTTPD,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::EVENT,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::BLOCK,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::SANDBOX,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::VERSION,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::KEYSTORE,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::BALANCER,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::ACCOUNT,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::RPC_CLIENT,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::RPC_SERVER,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::CONSENSUS_QUERY,
        keto::server_common::Events::CONSENSUS_SESSION_CHECK::MEMORY_VAULT_MANAGER};

const std::vector<std::string> Constants::CONSENSUS_HEARTBEAT = {
        keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_SERVER,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_CLIENT,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::BLOCK,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::TEST,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::TRANSACTION,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::ROUTER,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::HTTPD,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::EVENT,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::SANDBOX,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::VERSION,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::KEYSTORE,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::BALANCER,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::ACCOUNT,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::CONSENSUS_QUERY,
        keto::server_common::Events::CONSENSUS_HEARTBEAT::MEMORY_VAULT_MANAGER};

const std::vector<std::string> Constants::CONSENSUS_CONFIMATION_HEARTBEAT = {
            keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_SERVER,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::RPC_CLIENT,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::TEST,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::TRANSACTION,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::ROUTER,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::HTTPD,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::EVENT,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::SANDBOX,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::VERSION,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::KEYSTORE,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::BALANCER,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::ACCOUNT,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::CONSENSUS_QUERY,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::MEMORY_VAULT_MANAGER,
            keto::server_common::Events::CONSENSUS_HEARTBEAT::BLOCK};

const std::vector<std::string> Constants::CONSENSUS_SESSION_STATE = {
    keto::server_common::Events::CONSENSUS_SESSION_STATE::BLOCK,
    keto::server_common::Events::CONSENSUS_SESSION_STATE::MEMORY_VAULT_MANAGER
};

// network protocol configuration
const char* Constants::NETWORK_PROTOCOL_DELAY_CONFIGURATION = "network_protocol_delay";
const int Constants::NETWORK_PROTOCOL_DELAY_DEFAULT = 10;
const char* Constants::NETWORK_PROTOCOL_COUNT_CONFIGURATION = "network_protocol_count";
const int Constants::NETWORK_PROTOCOL_COUNT_DEFAULT = 6;

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

}
}