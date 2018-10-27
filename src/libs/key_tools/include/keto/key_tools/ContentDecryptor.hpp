/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ContentDecryptor.hpp
 * Author: ubuntu
 *
 * Created on June 19, 2018, 11:00 AM
 */

#ifndef CONTENTDECRYPTOR_HPP
#define CONTENTDECRYPTOR_HPP

#include <vector>
#include <string>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_tools {

class ContentDecryptor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ContentDecryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& encyptedContent);
    ContentDecryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& encyptedContent);
    ContentDecryptor(const ContentDecryptor& orig) = default;
    virtual ~ContentDecryptor();
    
    keto::crypto::SecureVector getContent();
    keto::crypto::SecureVector getDerivedKey(
            const keto::crypto::SecureVector& secret,
            const keto::crypto::SecureVector& encodedKey);
    operator keto::crypto::SecureVector(); 
    
private:
    keto::crypto::SecureVector content;
    
    
};


}
}


#endif /* CONTENTDECRYPTOR_HPP */

