//
// Created by Brett Chaldecott on 2019/01/07.
//

#include <botan/sha2_32.h>
#include <botan/loadstor.h>

#include "keto/crypto/ConfigurableSha256.hpp"


namespace keto {
namespace crypto {

std::string ConfigurableSha256::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

std::unique_ptr<Botan::HashFunction> ConfigurableSha256::copy_state() const {
    return std::unique_ptr<Botan::HashFunction>(new ConfigurableSha256(*this));
}

void ConfigurableSha256::clear() {
    MDx_HashFunction::clear();
    this->m_digest = this->c_digest;
}

void ConfigurableSha256::compress_digest(Botan::secure_vector<uint32_t>& digest,
                 const uint8_t input[],
                 size_t blocks) {
    Botan::SHA_256::compress_digest(digest,input,blocks);
}

void ConfigurableSha256::compress_n(const uint8_t input[], size_t blocks) {
    ConfigurableSha256::compress_digest(m_digest, input, blocks);
}

void ConfigurableSha256::copy_out(uint8_t output[]) {
    Botan::copy_out_vec_be(output, output_length(), m_digest);
}

}
}
