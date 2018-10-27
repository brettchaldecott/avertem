/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Resource.hpp
 * Author: ubuntu
 *
 * Created on February 25, 2018, 11:36 AM
 */

#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace transaction {

class Resource {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    virtual void commit() = 0;
    virtual void rollback() = 0;
    
};
    
}    
}


#endif /* RESOURCE_HPP */

