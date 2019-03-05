//
// Created by Brett Chaldecott on 2019/02/08.
//

#ifndef KETO_ACCOUNTTRANSACTIONINFOPROTOHELPER_HPP
#define KETO_ACCOUNTTRANSACTIONINFOPROTOHELPER_HPP

#include <string>
#include <memory>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/transaction_common/TransactionWrapperHelper.hpp"

namespace keto {
namespace transaction_common {

class AccountTransactionInfoProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    AccountTransactionInfoProtoHelper();
    AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockchainId,
                                      const TransactionWrapper_t& transactionWrapper);
    AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockchainId,
                                      const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr);
    AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockchainId, const keto::asn1::HashHelper& blockId,
                                      const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr);
    AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockchainId, const keto::asn1::HashHelper& blockId,
                                      TransactionWrapper_t* transactionWrapper);
    AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockchainId, const keto::asn1::HashHelper& blockId,
                                      const TransactionWrapper_t& transactionWrapper);
    AccountTransactionInfoProtoHelper(const keto::proto::AccountTransactionInfo& accountTransactionInfo);
    AccountTransactionInfoProtoHelper(const AccountTransactionInfoProtoHelper& accountTransactionInfoProtoHelper) = default;
    virtual ~AccountTransactionInfoProtoHelper();


    operator keto::proto::AccountTransactionInfo() const;
    operator std::string();

    AccountTransactionInfoProtoHelper& setBlockChainId(const keto::asn1::HashHelper& blockchainId);
    keto::asn1::HashHelper getBlockChainId();
    AccountTransactionInfoProtoHelper& setBlockId(const keto::asn1::HashHelper& blockId);
    keto::asn1::HashHelper getBlockId();
    AccountTransactionInfoProtoHelper& setTransaction(const keto::transaction_common::TransactionWrapperHelperPtr& transaction);
    keto::transaction_common::TransactionWrapperHelperPtr getTransaction() const;


private:
    keto::asn1::HashHelper blockchainId;
    keto::asn1::HashHelper blockId;
    keto::transaction_common::TransactionWrapperHelperPtr transaction;



};


}
}


#endif //KETO_ACCOUNTTRANSACTIONINFOPROTOHELPER_HPP
