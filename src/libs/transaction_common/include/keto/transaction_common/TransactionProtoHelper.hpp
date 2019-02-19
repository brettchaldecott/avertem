/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionProtoHelper.hpp
 * Author: ubuntu
 *
 * Created on March 19, 2018, 7:59 AM
 */

#ifndef TRANSACTIONPROTOHELPER_HPP
#define TRANSACTIONPROTOHELPER_HPP

#include <string>
#include <memory>

#include "BlockChain.pb.h"
#include "TransactionMessage.h"

#include "keto/transaction_common/TransactionEncryptionHandler.hpp"

#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace transaction_common {

class TransactionProtoHelper;
typedef std::shared_ptr<TransactionProtoHelper> TransactionProtoHelperPtr;

class TransactionProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    TransactionProtoHelper();
    TransactionProtoHelper(const keto::proto::Transaction& transaction);
    TransactionProtoHelper(const TransactionMessageHelperPtr& transactionMessageHelper);
    TransactionProtoHelper(const TransactionMessageHelperPtr& transactionMessageHelper,
            keto::transaction_common::TransactionEncryptionHandler& transactionEncryptionHandler);
    TransactionProtoHelper(const TransactionProtoHelper& orig) = default;
    virtual ~TransactionProtoHelper();
    
    TransactionProtoHelper& setTransaction(const TransactionMessageHelperPtr& transactionMessageHelper);
    TransactionProtoHelper& setTransaction(const TransactionMessageHelperPtr& transactionMessageHelper,
            keto::transaction_common::TransactionEncryptionHandler& transactionEncryptionHandler);
    TransactionProtoHelper& setTransaction(const std::string& buffer);
    
    operator std::string() const;
    operator keto::proto::Transaction&();
    TransactionProtoHelper& operator = (const keto::proto::Transaction& transaction);
    
    TransactionMessageHelperPtr getTransactionMessageHelper();

private:
    keto::proto::Transaction transaction;
};


}
}

#endif /* TRANSACTIONPROTOHELPER_HPP */

