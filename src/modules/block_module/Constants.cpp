/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.cpp
 * Author: ubuntu
 * 
 * Created on March 8, 2018, 8:09 AM
 */

#include "keto/block/Constants.hpp"

namespace keto {
namespace block {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

const char* Constants::GENESIS_CONFIG   = "genesis_config";

// keys for server
const char* Constants::PRIVATE_KEY      = "server-private-key";
const char* Constants::PUBLIC_KEY       = "server-public-key";

const char* Constants::BLOCK_PRODUCER_ENABLED
                                        = "block_producer_enabled";
const char* Constants::BLOCK_PRODUCER_ENABLED_TRUE
                                        = "true";

const char* Constants::BLOCK_PRODUCER_SAFE_MODE
                                        = "block_producer_safe_mode";
const char* Constants::BLOCK_PRODUCER_SAFE_MODE_ENABLED_TRUE
                                        = "true";
const long Constants::BLOCK_PRODUCER_SAFE_MODE_DELAY
                                        = 10;


const char* Constants::NETWORK_FEE_RATIO = "network_fee_ratio";
const long Constants::MAX_RUN_TIME      = 60 * 10;
const long Constants::SYNC_RETRY_DELAY_MIN  = 60 * 9;



const long Constants::SYNC_EXPIRY_TIME  = 60 * 5;


// tangle configuration
const int Constants::MAX_TANGLE_ACCOUNTS = 50;
const int Constants::MAX_TANGLES_TO_ACCOUNT = 50;

// the delay values
const int Constants::ACTIVATE_PRODUCER_DELAY    = 40;
const int Constants::BLOCK_TIME                 = 15;
const int Constants::BLOCK_PRDUCER_DEACTIVATE_CHECK_DELAY    = 10;
const int Constants::BLOCK_PRODUCER_RETRY_MAX   = 4;

}
}
