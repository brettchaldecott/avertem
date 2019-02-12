/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on February 27, 2018, 7:50 AM
 */

#ifndef BLOCK_CONSTANTS_HPP
#define BLOCK_CONSTANTS_HPP

#include <vector>
#include <string>

#include "keto/asn1/HashHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace block_db {


class
Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    // string constants
    static const char* BLOCK_META_INDEX;
    static const char* BLOCKS_INDEX;
    static const char* TRANSACTIONS_INDEX;
    static const char* ACCOUNTS_INDEX;
    static const char* CHILD_INDEX;
    
    // boot constants
    static const char* MASTER_CHAIN_KEY;
    static const keto::asn1::HashHelper MASTER_CHAIN_HASH;
    static const char* GENESIS_KEY;
    static const keto::asn1::HashHelper GENESIS_HASH;
    
    // parent block
    static const char* PARENT_KEY;
    static const char* BLOCK_COUNT;
    
    static const std::vector<std::string> DB_LIST;
    
    
    
};


}
}

#endif /* CONSTANTS_HPP */

