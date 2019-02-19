/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionMessageHelper.hpp
 * Author: ubuntu
 *
 * Created on March 17, 2018, 4:21 AM
 */

#ifndef TRANSACTIONMESSAGEHELPER_HPP
#define TRANSACTIONMESSAGEHELPER_HPP

#include <string>
#include <memory>
#include <vector>

#include "Transaction.h"
#include "SignedTransaction.h"
#include "SignedChangeSet.h"
#include "TransactionWrapper.h"
#include "TransactionMessage.h"
#include "TransactionTrace.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

#include "keto/transaction_common/SignedTransactionHelper.hpp"
#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/transaction_common/TransactionEncryptionHandler.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace transaction_common {

class TransactionMessageHelper;
typedef std::shared_ptr<TransactionMessageHelper> TransactionMessageHelperPtr;

class TransactionMessageHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    TransactionMessageHelper();
    TransactionMessageHelper(const TransactionWrapperHelperPtr& transactionWrapper);
    TransactionMessageHelper(TransactionWrapper_t* transactionWrapper);
    TransactionMessageHelper(TransactionMessage_t* transactionMessage);
    TransactionMessageHelper(const std::string& transactionMessage);
    TransactionMessageHelper(const TransactionMessageHelper& orig);
    virtual ~TransactionMessageHelper();
    
    TransactionMessageHelper& setTransactionWrapper(TransactionWrapper_t* transactionWrapper);
    TransactionMessageHelper& setTransactionWrapper(const TransactionWrapperHelperPtr& transactionWrapper);
    TransactionWrapperHelperPtr getTransactionWrapper();
    TransactionMessageHelper& addNestedTransaction(TransactionMessage_t* nestedTransaction);
    TransactionMessageHelper& addNestedTransaction(const TransactionMessageHelperPtr& nestedTransaction);
    TransactionMessageHelper& addNestedTransaction(const TransactionMessageHelper& nestedTransaction);
    std::vector<TransactionMessageHelperPtr> getNestedTransactions();
    
    
    TransactionMessageHelper& operator =(const std::string& transactionMessage);
    operator TransactionMessage_t&();
    operator TransactionMessage_t*();
    operator ANY_t*();
    
    operator std::vector<uint8_t>();
    
    std::vector<uint8_t> serializeTransaction(TransactionEncryptionHandler& 
            transactionEncryptionHandler);
    
    TransactionMessage_t* getMessage(TransactionEncryptionHandler& 
            transactionEncryptionHandler);
    
    TransactionMessageHelperPtr decryptMessage(TransactionEncryptionHandler&
            transactionEncryptionHandler);
    
    bool isEncrypted();

    // the number of seconds available to this contract
    time_t getAvailableTime();
    TransactionMessageHelper& setAvailableTime(time_t availableTime);
    time_t getElapsedTime();
    TransactionMessageHelper& setElapsedTime(time_t elapsedTime);
    
    
private:
    TransactionMessage_t* transactionMessage;
    std::vector<TransactionMessageHelperPtr> nestedTransactions;
    
    
    void getMessage(TransactionEncryptionHandler& 
            transactionEncryptionHandler,TransactionMessage_t* transactionMessage);
    
    void decryptMessage(TransactionEncryptionHandler&
        transactionEncryptionHandler, TransactionMessageHelperPtr transactionMessageHelperPtr, 
        TransactionMessage__nestedTransactions__Member* transactionMessage);
};


}
}

#endif /* TRANSACTIONMESSAGEHELPER_HPP */

