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

#include "keto/election_common/Constants.hpp"

#include "keto/server_common/Events.hpp"

namespace keto {
namespace election_common {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


const std::vector<std::string> Constants::ELECTION_INTERNAL_PUBLISH         ={
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_PUBLISH,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_CLIENT,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_SERVER,
        keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_PUBLISH};
const std::vector<std::string> Constants::ELECTION_PROCESS_PUBLISH          ={
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_PUBLISH,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_CLIENT,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PUBLISH_SERVER,
        keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_PUBLISH};
const std::vector<std::string> Constants::ELECTION_PROCESS_CONFIRMATION     ={
        keto::server_common::Events::ROUTER_QUERY::ELECT_RPC_PROCESS_CONFIRMATION,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_CLIENT,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CONFIRMATION_SERVER,
        keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_PROCESS_CONFIRMATION};


}
}