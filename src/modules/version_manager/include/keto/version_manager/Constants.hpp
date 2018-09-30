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

#ifndef VERSION_MANAGER_CONSTANTS_HPP
#define VERSION_MANAGER_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace version_manager {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    // string constants
    static const char* CHECK_SCRIPT;
    static const char* AUTO_UPDATE;
};


}
}

#endif /* CONSTANTS_HPP */

