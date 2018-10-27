/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EncryptionHandler.hpp
 * Author: ubuntu
 *
 * Created on October 11, 2018, 11:14 AM
 */

#ifndef TRANSACTION_ENCRYPTION_HANDLER_HPP
#define TRANSACTION_ENCRYPTION_HANDLER_HPP

#include <string>
#include <memory>

#include "EncryptedDataWrapper.h"
#include "TransactionMessage.h"

namespace keto {
namespace transaction_common {

class TransactionEncryptionHandler;
typedef std::shared_ptr<TransactionEncryptionHandler> TransactionEncryptionHandlerPtr;

class TransactionEncryptionHandler {
public:
    
    virtual EncryptedDataWrapper_t* encrypt(
            const TransactionMessage_t& transaction) = 0;
    
    virtual TransactionMessage_t* decrypt(
            const EncryptedDataWrapper_t& encrypt) = 0;
    
};
    
}
}

#endif /* ENCRYPTIONHANDLER_HPP */

