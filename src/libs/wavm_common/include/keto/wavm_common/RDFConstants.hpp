/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RDFConstants.hpp
 * Author: ubuntu
 *
 * Created on May 16, 2018, 6:08 AM
 */

#ifndef RDFCONSTANTS_HPP
#define RDFCONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace wavm_common {

class RDFConstants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    RDFConstants() = delete;
    RDFConstants(const RDFConstants& orig) = delete;
    virtual ~RDFConstants() = delete;

    static const char* CHANGE_SET_SUBJECT;

    class CHANGE_SET_PREDICATES {
    public:
        static const char* ID;
        static const char* CHANGE_SET_HASH;
        static const char* TYPE;
        static const char* DATE_TIME;
        static const char* URI;
    };

    class ACCOUNT_TRANSACTION_PREDICATES {
    public:
        static const char* ID;
        static const char* ACCOUNT_HASH;
        static const char* NAME;
        static const char* DESCRIPTION;
        static const char* TYPE;
        static const char* DATE_TIME;
        static const char* VALUE;
    };
    
    class TYPES {
    public:
        static const char* STRING;
        static const char* LONG;
        static const char* FLOAT;
        static const char* BOOLEAN;
        static const char* DATE_TIME;
    };
    
    class NODE_TYPES{
    public:
        static const char* LITERAL;
    };
private:

};


}
}


#endif /* RDFCONSTANTS_HPP */

