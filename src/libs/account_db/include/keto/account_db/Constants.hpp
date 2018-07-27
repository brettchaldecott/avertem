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

#ifndef ACCOUNT_DB_CONSTANTS_HPP
#define ACCOUNT_DB_CONSTANTS_HPP

#include <vector>
#include <string>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace account_db {


class
Constants {
public:
    static std::string getVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();
    
    // string constants
    static const char* ACCOUNTS_MAPPING;
    static const char* BASE_GRAPH;
    static const char* GRAPH_BASE_DIR;
    
    static const std::vector<std::string> DB_LIST;
    
};


}
}

#endif /* CONSTANTS_HPP */

