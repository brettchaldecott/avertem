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

const char* Constants::GENESIS_CONFIG = "genesis_config";

// keys for server
const char* Constants::PRIVATE_KEY    = "server-private-key";
const char* Constants::PUBLIC_KEY     = "server-public-key";

const char* Constants::BLOCK_PRODUCER_ENABLED
                                      = "block_producer_enabled";
const char* Constants::BLOCK_PRODUCER_ENABLED_TRUE
                                      = "true";

}
}
