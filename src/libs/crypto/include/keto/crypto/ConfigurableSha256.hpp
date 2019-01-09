//
// Created by Brett Chaldecott on 2019/01/07.
//

#ifndef KETO_CONFIGURABLE_SHA256_HPP
#define KETO_CONFIGURABLE_SHA256_HPP

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

class ConfigurableSha256 : public Botan::MDx_HashFunction {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    std::string name() const override { return "SHA-256"; }
    size_t output_length() const override { return 32; }
    HashFunction* clone() const override { return new ConfigurableSha256(c_digest); }
    std::unique_ptr<HashFunction> copy_state() const override;

    void clear() override;

    ConfigurableSha256(Botan::secure_vector<uint32_t> digest) : MDx_HashFunction(64, true, true), c_digest(digest), m_digest(digest)
    { clear();}

    /*
    * Perform a SHA-256 compression. For internal use
    */
    static void compress_digest(Botan::secure_vector<uint32_t>& digest,
                                const uint8_t input[],
                                size_t blocks);

private:
    Botan::secure_vector<uint32_t> c_digest;
    Botan::secure_vector<uint32_t> m_digest;

    void compress_n(const uint8_t input[], size_t blocks) override;
    void copy_out(uint8_t output[]) override;

};

}
}

#endif //KETO_RANDOMSHA256_HPP
