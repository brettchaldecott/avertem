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
    keto::server_common::Events::CONSENSUS::RPC_SERVER};

const char* Constants::CPP_FILE_VERSION = "$Id$";

}
}