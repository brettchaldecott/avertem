/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on January 16, 2018, 12:08 PM
 */

#ifndef KETO_MODULE_CONSTANTS_HPP
#define KETO_MODULE_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace module {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static constexpr const char* KETO_SHARE = "shared";
    static constexpr const char* KETO_TMP = "tmp";
    
private:
};
    
}
}


#endif /* CONSTANTS_HPP */

