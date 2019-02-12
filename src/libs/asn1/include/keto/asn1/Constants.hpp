/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on March 12, 2018, 11:02 AM
 */

#ifndef ASN1_CONSTANTS_HPP
#define ASN1_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace asn1 {

class
Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static constexpr const char* RDF_LANGUAGE = "en";
    class RDF_NODE {
    public:
        static constexpr const char* LITERAL = "literal";
        static constexpr const char* URI = "uri";
    };

    class RDF_TYPES {
    public:
        static constexpr const char* STRING = "http://www.w3.org/2001/XMLSchema#string";
        static constexpr const char* LONG = "http://www.w3.org/2001/XMLSchema#decimal";
        static constexpr const char* FLOAT = "http://www.w3.org/2001/XMLSchema#float";
        static constexpr const char* BOOLEAN = "http://www.w3.org/2001/XMLSchema#Boolean";
        static constexpr const char* DATE_TIME = "http://www.w3.org/2001/XMLSchema#dateTime";
    };
    
};


}
}


#endif /* CONSTANTS_HPP */

