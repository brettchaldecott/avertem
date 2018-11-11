/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpSessionTransactionEncryptor.hpp
 * Author: ubuntu
 *
 * Created on November 6, 2018, 11:53 AM
 */

#ifndef HTTPSESSIONTRANSACTIONENCRYPTOR_HPP
#define HTTPSESSIONTRANSACTIONENCRYPTOR_HPP

// std includes
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include "keto/transaction_common/TransactionEncryptionHandler.hpp"
#include "keto/crypto/KeyLoader.hpp"


namespace keto {
namespace session {
    
class HttpSessionTransactionEncryptor;
typedef std::shared_ptr<HttpSessionTransactionEncryptor> HttpSessionTransactionEncryptorPtr;

class HttpSessionTransactionEncryptor : public keto::transaction_common::TransactionEncryptionHandler {
public:
    HttpSessionTransactionEncryptor(const keto::crypto::KeyLoaderPtr& keyLoaderPtr,
            const std::vector<uint8_t>& publicKey);
    HttpSessionTransactionEncryptor(const HttpSessionTransactionEncryptor& orig) = delete;
    virtual ~HttpSessionTransactionEncryptor();
    

    EncryptedDataWrapper_t* encrypt(
            const TransactionMessage_t& transaction);
    
    TransactionMessage_t* decrypt(
            const EncryptedDataWrapper_t& encrypt);

    
private:
    keto::crypto::KeyLoaderPtr keyLoaderPtr;
    std::vector<uint8_t> publicKey;
};

}
}

#endif /* HTTPSESSIONTRANSACTIONENCRYPTOR_HPP */

