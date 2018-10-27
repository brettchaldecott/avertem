/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: Brett Chaldecott
 *
 * Created on February 6, 2018, 11:09 AM
 */

#ifndef CRYPTO_CONSTANTS_HPP
#define CRYPTO_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace crypto {


class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    
    static constexpr const char* SIGNATURE_TYPE = "EMSA3(SHA-256)";
    static constexpr const char* HASH_TYPE = "SHA-256";
    
};

}
}

#endif /* CONSTANTS_HPP */

