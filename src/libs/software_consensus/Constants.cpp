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
    keto::server_common::Events::CONSENSUS::SANBOX,
    keto::server_common::Events::CONSENSUS::VERSION,
    keto::server_common::Events::CONSENSUS::KEYSTORE,
    keto::server_common::Events::CONSENSUS::BALANCER,
    keto::server_common::Events::CONSENSUS::ACCOUNT,
    keto::server_common::Events::CONSENSUS::RPC_CLIENT,
    keto::server_common::Events::CONSENSUS::RPC_SERVER,
    keto::server_common::Events::CONSENSUS::CONSENSUS_QUERY};

const std::vector<std::string> Constants::CONSENSUS_SESSION_ORDER = {
    keto::server_common::Events::CONSENSUS_SESSION::TEST,
    keto::server_common::Events::CONSENSUS_SESSION::TRANSACTION,
    keto::server_common::Events::CONSENSUS_SESSION::ROUTER,
    keto::server_common::Events::CONSENSUS_SESSION::HTTPD,
    keto::server_common::Events::CONSENSUS_SESSION::EVENT,
    keto::server_common::Events::CONSENSUS_SESSION::BLOCK,
    keto::server_common::Events::CONSENSUS_SESSION::SANBOX,
    keto::server_common::Events::CONSENSUS_SESSION::VERSION,
    keto::server_common::Events::CONSENSUS_SESSION::KEYSTORE,
    keto::server_common::Events::CONSENSUS_SESSION::BALANCER,
    keto::server_common::Events::CONSENSUS_SESSION::ACCOUNT,
    keto::server_common::Events::CONSENSUS_SESSION::RPC_CLIENT,
    keto::server_common::Events::CONSENSUS_SESSION::CONSENSUS_QUERY};
//const keto::obfuscate::MetaString Constants::CPP_FILE_VERSION = DEF_OBFUSCATED("$Id: 91742e32879ab84609ca48fbfbaccca91dca7257 $");

    std::string Constants::getSourceVersion() {
        return OBFUSCATED("$Id: 91742e32879ab84609ca48fbfbaccca91dca7257 $");
    }

}
}