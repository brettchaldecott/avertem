//
// Created by Brett Chaldecott on 2019/01/07.
//

#ifndef KETO_PASSWORDPIPELINE_HPP
#define KETO_PASSWORDPIPELINE_HPP

#include <string>
#include <memory>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/mdx_hash.h>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/crypto/ConfigurableSha256.hpp"
#include "keto/crypto/Containers.hpp"


namespace keto {
namespace crypto {

class PasswordPipeLine;
typedef std::shared_ptr<PasswordPipeLine> PasswordPipeLinePtr;

class PasswordPipeLine {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    PasswordPipeLine();
    PasswordPipeLine(const PasswordPipeLine& orig) = delete;
    virtual ~PasswordPipeLine();

    keto::crypto::SecureVector generatePassword(const keto::crypto::SecureVector& password);
private:
    std::vector<ConfigurableSha256> shas;
};


}
}

#endif //KETO_PASSWORDPIPELINE_HPP
