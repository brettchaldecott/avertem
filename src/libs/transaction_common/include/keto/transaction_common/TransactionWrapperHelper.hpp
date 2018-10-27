/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionWrapperHelper.hpp
 * Author: ubuntu
 *
 * Created on March 17, 2018, 4:21 AM
 */

#ifndef TRANSACTIONWRAPPERHELPER_HPP
#define TRANSACTIONWRAPPERHELPER_HPP

#include <string>
#include <memory>
#include <vector>

#include "Transaction.h"
#include "SignedTransaction.h"
#include "SignedChangeSet.h"
#include "TransactionWrapper.h"
#include "TransactionTrace.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

#include "keto/transaction_common/SignedTransactionHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace transaction_common {

class TransactionWrapperHelper;
typedef std::shared_ptr<TransactionWrapperHelper> TransactionWrapperHelperPtr;

class TransactionWrapperHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    TransactionWrapperHelper();
    TransactionWrapperHelper(SignedTransaction_t* signedTransaction);
    TransactionWrapperHelper(SignedTransaction_t* signedTransaction,
            const keto::asn1::HashHelper& sourceAccount, 
            const keto::asn1::HashHelper& targetAccount);
    TransactionWrapperHelper(TransactionWrapper_t* transactionWrapper);
    TransactionWrapperHelper(TransactionWrapper_t* transactionWrapper, bool own);
    TransactionWrapperHelper(const std::string& transactionWrapper);
    TransactionWrapperHelper(const TransactionWrapperHelper& orig) = delete;
    virtual ~TransactionWrapperHelper();
    
    TransactionWrapperHelper& setSignedTransaction(SignedTransaction_t* signedTransaction);
    TransactionWrapperHelper& setSourceAccount(const keto::asn1::HashHelper& sourceAccount);
    TransactionWrapperHelper& setTargetAccount(const keto::asn1::HashHelper& targetAccount);
    TransactionWrapperHelper& setFeeAccount(const keto::asn1::HashHelper& feeAccount);
    TransactionWrapperHelper& setStatus(const Status& status);
    TransactionWrapperHelper& addTransactionTrace(TransactionTrace_t* transactionTrace);
    TransactionWrapperHelper& addChangeSet(SignedChangeSet_t* signedChangeSet);
    
    TransactionWrapperHelper& operator =(const std::string& transactionWrapper);
    operator TransactionWrapper_t&();
    operator TransactionWrapper_t*();
    operator ANY_t*();
    
    operator std::vector<uint8_t>();
    
    keto::asn1::HashHelper getSourceAccount();
    keto::asn1::HashHelper getTargetAccount();
    keto::asn1::HashHelper getFeeAccount();
    keto::asn1::HashHelper getCurrentAccount();
    keto::asn1::HashHelper getHash();
    keto::asn1::SignatureHelper getSignature();
    Status getStatus();
    Status incrementStatus();
    SignedTransactionHelperPtr getSignedTransaction();
    
private:
    bool own;
    TransactionWrapper_t* transactionWrapper;
};


}
}

#endif /* TRANSACTIONMESSAGEHELPER_HPP */

