/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HashBuilder.hpp
 * Author: ubuntu
 *
 * Created on June 1, 2018, 10:10 AM
 */

#ifndef HASHBUILDER_HPP
#define HASHBUILDER_HPP

#include <string>

#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/Containers.hpp"

namespace keto {
namespace software_consensus {


class HashBuilder {
public:
    HashBuilder();
    HashBuilder(const HashBuilder& orig) = default;
    virtual ~HashBuilder();
    
    HashBuilder& operator << (const std::string& value);
    HashBuilder& operator << (const keto::crypto::SecureVector& value);
    HashBuilder& operator << (char value);
    HashBuilder& operator << (long value);
    HashBuilder& operator << (float value);
    
    keto::crypto::SecureVector generateHash();
private:
    keto::crypto::SecureVector secureVector;
};


}
}

#endif /* HASHBUILDER_HPP */

