//
// Created by Brett Chaldecott on 2019/04/04.
//

#ifndef KETO_SECP256K1UTILS_HPP
#define KETO_SECP256K1UTILS_HPP

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
#include "keto/server_common/ModuleSession.hpp"


namespace keto {
namespace crypto {

class Secp256K1Utils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    class Secp256K1Memory : public keto::server_common::ModuleSession {
    public:
        Secp256K1Memory();
        Secp256K1Memory(const Secp256K1Memory& orig) = delete;
        virtual ~Secp256K1Memory();

    };

    class Secp256K1UtilsScope {
    public:
        Secp256K1UtilsScope();
        Secp256K1UtilsScope(const Secp256K1UtilsScope& orig) = delete;
        virtual ~Secp256K1UtilsScope();

        static void init();
        static void fin();
    };
    typedef std::shared_ptr<Secp256K1UtilsScope> Secp256K1UtilsScopePtr;

    static bool verifySignature(const std::vector<uint8_t>& bits, std::vector<uint8_t> message, const std::vector<uint8_t>& signature);

private:

};


}
}



#endif //KETO_SECP256K1UTILS_HPP
