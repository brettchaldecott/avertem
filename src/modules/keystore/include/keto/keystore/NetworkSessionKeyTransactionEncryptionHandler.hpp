//
// Created by Brett Chaldecott on 2019/02/01.
//

#ifndef KETO_NETWORKSESSIONKEYTRANSACTIONENCRYPTIONHANDLER_HPP
#define KETO_NETWORKSESSIONKEYTRANSACTIONENCRYPTIONHANDLER_HPP

// std includes
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include "keto/transaction_common/TransactionEncryptionHandler.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/keystore/NetworkSessionKeyManager.hpp"

namespace keto {
namespace keystore {

class NetworkSessionKeyTransactionEncryptionHandler;
typedef std::shared_ptr<NetworkSessionKeyTransactionEncryptionHandler> NetworkSessionKeyTransactionEncryptionHandlerPtr;

class NetworkSessionKeyTransactionEncryptionHandler : public keto::transaction_common::TransactionEncryptionHandler {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    NetworkSessionKeyTransactionEncryptionHandler();
    NetworkSessionKeyTransactionEncryptionHandler(const NetworkSessionKeyTransactionEncryptionHandler& orig) = delete;
    virtual ~NetworkSessionKeyTransactionEncryptionHandler();


    EncryptedDataWrapper_t* encrypt(
            const TransactionMessage_t& transaction);

    TransactionMessage_t* decrypt(
            const EncryptedDataWrapper_t& encrypt);


private:

    std::vector<uint8_t> copyEncryptedToVector(
            const EncryptedDataWrapper_t& encrypt);
};

}
}

#endif //KETO_NETWORKSESSIONKEYTRANSACTIONENCRYPTIONHANDLER_HPP
