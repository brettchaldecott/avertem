/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignatureGenerator.hpp
 * Author: ubuntu
 *
 * Created on February 6, 2018, 11:27 AM
 */

#ifndef SIGNATURE_GENERATOR_HPP
#define SIGNATURE_GENERATOR_HPP

#ifndef BOTAN_NO_DEPRECATED_WARNINGS
#define BOTAN_NO_DEPRECATED_WARNINGS
#endif

#include "keto/crypto/Constants.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/crypto/KeyLoader.hpp"

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/ecdsa.h>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace crypto {


class SignatureGenerator {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    SignatureGenerator(const keto::crypto::SecureVector& key);
    SignatureGenerator(const keto::crypto::KeyLoaderPtr& loader);
    SignatureGenerator(const SignatureGenerator& orig) = default;
    virtual ~SignatureGenerator();
    
    std::vector<uint8_t> sign(std::vector<uint8_t>& value);
    std::vector<uint8_t> sign(const keto::crypto::SecureVector& value);
    
private:
    keto::crypto::SecureVector key;
    keto::crypto::KeyLoaderPtr loader;

    std::shared_ptr<Botan::Private_Key> loadKey(std::shared_ptr<Botan::RandomNumberGenerator> rng);
};


}
}
#endif /* SIGNATURETOOL_HPP */

