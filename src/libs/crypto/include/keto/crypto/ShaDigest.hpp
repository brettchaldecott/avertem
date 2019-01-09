//
// Created by Brett Chaldecott on 2019/01/07.
//

#ifndef KETO_SHADIGEST_HPP
#define KETO_SHADIGEST_HPP

#include <string>
#include <memory>

#include <boost/filesystem/path.hpp>

#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/mdx_hash.h>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace crypto {

class ShaDigest {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ShaDigest();
    ShaDigest(const ShaDigest& orig) = default;
    virtual ~ShaDigest();

    Botan::secure_vector<uint32_t> getDigest();
private:
    Botan::secure_vector<uint32_t> m_digest;

};


}
}


#endif //KETO_SHADIGEST_HPP
