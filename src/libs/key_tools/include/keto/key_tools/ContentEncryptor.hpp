/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ContentEncryptor.hpp
 * Author: ubuntu
 *
 * Created on June 21, 2018, 3:02 AM
 */

#ifndef CONTENTENCRYPTOR_HPP
#define CONTENTENCRYPTOR_HPP

#include <vector>
#include <string>

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace key_tools {


class ContentEncryptor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ContentEncryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const std::vector<uint8_t>& content);
    ContentEncryptor(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
        const keto::crypto::SecureVector& content);
    ContentEncryptor(const ContentEncryptor& orig) = default;
    virtual ~ContentEncryptor();

    std::vector<uint8_t> getEncryptedContent();
    keto::crypto::SecureVector getEncryptedContent_locked();
    operator std::vector<uint8_t>(); 

    
private:
    std::vector<uint8_t> encyptedContent;
};


}
}

#endif /* CONTENTENCRYPTOR_HPP */

