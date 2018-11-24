/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on March 8, 2018, 8:09 AM
 */

#ifndef BLOCK_MODULE_CONSTANTS_HPP
#define BLOCK_MODULE_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace block {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    // string constants
    static const char* GENESIS_CONFIG;
    
    // keys for server
    static const char* PRIVATE_KEY;
    static const char* PUBLIC_KEY;
    
    // block producer
    static const char* BLOCK_PRODUCER_ENABLED;
    static const char* BLOCK_PRODUCER_ENABLED_TRUE;
};


}
}

#endif /* CONSTANTS_HPP */

