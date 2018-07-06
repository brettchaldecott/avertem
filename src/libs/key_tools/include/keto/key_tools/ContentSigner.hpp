/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ContentSigner.hpp
 * Author: ubuntu
 *
 * Created on July 5, 2018, 6:21 PM
 */

#ifndef CONTENTSIGNER_HPP
#define CONTENTSIGNER_HPP

#include <vector>
#include <string>

#include "keto/crypto/Containers.hpp"


namespace keto {
namespace key_tools {


class ContentSigner {
public:
    ContentSigner(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
            const std::vector<uint8_t>& content);
    ContentSigner(const keto::crypto::SecureVector& secret, const keto::crypto::SecureVector& encodedKey,
            const keto::crypto::SecureVector& content);
    ContentSigner(const ContentSigner& orig) = default;
    virtual ~ContentSigner();
    
    keto::crypto::SecureVector getSignature();
private:
    keto::crypto::SecureVector signature;
};

}
}


#endif /* CONTENTSIGNER_HPP */

