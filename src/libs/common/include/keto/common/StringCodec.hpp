/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StringCodec.hpp
 * Author: Brett Chaldecott
 *
 * Created on February 3, 2018, 7:29 AM
 */

#ifndef STRINGCODEC_HPP
#define STRINGCODEC_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace common {

class StringCodec {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
};
enum StringEncoding {
    BASE64,
    HEX,
    UTF8
};


}
}


#endif /* STRINGCODEC_HPP */

