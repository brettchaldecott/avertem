//
// Created by Brett Chaldecott on 2019/01/07.
//

#include "keto/crypto/PasswordPipeLine.hpp"
#include "keto/crypto/ConfigurableSha256.hpp"
#include "keto/crypto/ShaDigest.hpp"

namespace keto {
namespace crypto {


std::string PasswordPipeLine::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


PasswordPipeLine::PasswordPipeLine() {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,50);

    distribution(stdGenerator);

    int iterations = 50 + distribution(stdGenerator);

    for (int index = 0; index < iterations; index++) {
        ShaDigest shaDigest;
        ConfigurableSha256 configurableSha256(shaDigest.getDigest());
        shas.push_back(configurableSha256);
    }

}

PasswordPipeLine::~PasswordPipeLine() {

}

keto::crypto::SecureVector PasswordPipeLine::generatePassword(const keto::crypto::SecureVector& password) {
    std::lock_guard<std::mutex> guard(classMutex);
    keto::crypto::SecureVector bytes = password;
    for (ConfigurableSha256& configurableSha256: this->shas) {
        bytes = configurableSha256.process(bytes);
    }
    return bytes;
}

}
}
