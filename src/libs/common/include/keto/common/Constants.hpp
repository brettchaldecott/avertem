/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: Brett Chaldecott
 *
 * Created on February 13, 2018, 12:52 PM
 */

#ifndef KETO_COMMON_CONSTANTS_HPP
#define KETO_COMMON_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace common {


class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    
    static const int HTTP_VERSION = 11;
    
    static const char* CONTENT_TYPE_HEADING;
    static const char* PROTOBUF_CONTENT_TYPE;
    
};


}
}


#endif /* CONSTANTS_HPP */

