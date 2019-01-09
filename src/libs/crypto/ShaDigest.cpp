//
// Created by Brett Chaldecott on 2019/01/07.
//

#include <random>
#include <algorithm>

#include <botan/rng.h>
#include <botan/auto_rng.h>


#include "keto/crypto/ShaDigest.hpp"



namespace keto {
namespace crypto {

std::string ShaDigest::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ShaDigest::ShaDigest() : m_digest(8) {

    std::shared_ptr<Botan::AutoSeeded_RNG> generator(new Botan::AutoSeeded_RNG());

    m_digest[0] = generator->next_byte() ^ 0x6A09E667;
    m_digest[1] = generator->next_byte() ^ 0xBB67AE85;
    m_digest[2] = generator->next_byte() ^ 0x3C6EF372;
    m_digest[3] = generator->next_byte() ^ 0xA54FF53A;
    m_digest[4] = generator->next_byte() ^ 0x510E527F;
    m_digest[5] = generator->next_byte() ^ 0x9B05688C;
    m_digest[6] = generator->next_byte() ^ 0x1F83D9AB;
    m_digest[7] = generator->next_byte() ^ 0x5BE0CD19;
}

ShaDigest::~ShaDigest() {
}

Botan::secure_vector<uint32_t> ShaDigest::getDigest() {
    return this->m_digest;
}

}

}
