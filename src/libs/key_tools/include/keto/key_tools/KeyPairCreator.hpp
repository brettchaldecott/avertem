/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   KeyPairCreator.hpp
 * Author: ubuntu
 *
 * Created on June 21, 2018, 3:04 AM
 */

#ifndef KEYPAIRCREATOR_HPP
#define KEYPAIRCREATOR_HPP

#include <vector>
#include <string>

#include "keto/crypto/Containers.hpp"


namespace keto {
namespace key_tools {

class KeyPairCreator {
public:
    KeyPairCreator();
    KeyPairCreator(const KeyPairCreator& orig) = default;
    virtual ~KeyPairCreator();
    
    keto::crypto::SecureVector getSecret();
    keto::crypto::SecureVector getEncodedKey();
    
private:
    keto::crypto::SecureVector secret;
    keto::crypto::SecureVector encodedKey;
    
};


}
}


#endif /* KEYPAIRCREATOR_HPP */

