/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: Brett Chaldecott
 *
 * Created on February 6, 2018, 11:09 AM
 */

#ifndef CRYPTO_CONSTANTS_HPP
#define CRYPTO_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace crypto {


class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    
    static constexpr const char* SIGNATURE_TYPE = "EMSA3(SHA-256)";
    static constexpr const char* SECP256K_SIGNATURE_TYPE = "SECP256K";

    static constexpr const char* EMSA1_SIGNATURE_TYPE = "EMSA1(SHA-256)";

    static constexpr const char* EMSA3_RAW_SIGNATURE_TYPE = "EMSA3(Raw)";
    static constexpr const char* EMSA3_SHA1_SIGNATURE_TYPE = "EMSA3(SHA-1)";
    static constexpr const char* EMSA3_PKCS1_SIGNATURE_TYPE = "EMSA_PKCS1(SHA-256)";

    static constexpr const char* EMSA4_RAW_SIGNATURE_TYPE = "EMSA4(Raw)";
    static constexpr const char* EMSA4_SHA1_SIGNATURE_TYPE = "EMSA4(SHA-1)";
    static constexpr const char* EMSA4_SHA256_SIGNATURE_TYPE = "EMSA4(SHA-256)";
    static constexpr const char* EMSA4_SHA256_MGF1_SIGNATURE_TYPE = "EMSA4(SHA-256,MGF1,32)";

    static constexpr const char* PSSR_SHA256_MGF1_SIGNATURE_TYPE = "PSSR(SHA-256,MGF1,32)";

    static constexpr const char* HASH_TYPE = "SHA-256";
    static constexpr const char* ENCRYPTION_PADDING = "EME1(SHA-256)";
    static constexpr const char* CIPHER_STREAM = "ChaCha(20)";
    
};

}
}

#endif /* CONSTANTS_HPP */

