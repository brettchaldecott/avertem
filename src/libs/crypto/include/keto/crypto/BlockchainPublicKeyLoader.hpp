//
// Created by Brett Chaldecott on 2019/03/27.
//

#ifndef KETO_BLOCKCHAINKEYLOADER_HPP
#define KETO_BLOCKCHAINKEYLOADER_HPP

#include <string>
#include <memory>

#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace crypto {

class BlockchainPublicKeyLoader {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    BlockchainPublicKeyLoader(const std::string& hexString);
    BlockchainPublicKeyLoader(const std::vector<uint8_t>& bytes);
    BlockchainPublicKeyLoader(const BlockchainPublicKeyLoader& orig) = delete;
    virtual ~BlockchainPublicKeyLoader();


    std::shared_ptr<Botan::Public_Key> getPublicKey();

private:
    std::shared_ptr<Botan::Public_Key> publicKeyPtr;
};


}
}



#endif //KETO_BLOCKCHAINKEYLOADER_HPP
