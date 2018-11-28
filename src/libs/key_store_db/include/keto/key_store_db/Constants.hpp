/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: brettchaldecott
 *
 * Created on 24 November 2018, 7:18 AM
 */

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <vector>
#include <string>

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace key_store_db {

class Constants {
public:
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    // string constants
    static const char* KEY_STORE;

    static const std::vector<std::string> DB_LIST;

    // encryption padding
    static constexpr const char* ENCRYPTION_PADDING = "EME1(SHA-256)";



private:

};


}
}


#endif /* CONSTANTS_HPP */

