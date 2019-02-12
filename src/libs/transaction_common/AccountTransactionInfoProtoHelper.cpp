//
// Created by Brett Chaldecott on 2019/02/08.
//

#include "keto/transaction_common/AccountTransactionInfoProtoHelper.hpp"



namespace keto {
namespace transaction_common {

std::string AccountTransactionInfoProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountTransactionInfoProtoHelper::AccountTransactionInfoProtoHelper() {

}

AccountTransactionInfoProtoHelper::AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockchainId, const keto::asn1::HashHelper& blockId,
                              const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr) :
                              blockchainId(blockchainId), blockId(blockId), transaction(transactionWrapperHelperPtr) {

}

AccountTransactionInfoProtoHelper::AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockChainId,
        const keto::asn1::HashHelper& blockId, TransactionWrapper_t* transactionWrapper) :
        blockchainId(blockchainId), blockId(blockId), transaction(new keto::transaction_common::TransactionWrapperHelper(transactionWrapper)){
}


AccountTransactionInfoProtoHelper::AccountTransactionInfoProtoHelper(const keto::asn1::HashHelper& blockChainId, const keto::asn1::HashHelper& blockId,
                              const TransactionWrapper_t& transactionWrapper) :
        blockchainId(blockchainId), blockId(blockId), transaction(new keto::transaction_common::TransactionWrapperHelper(transactionWrapper)){

}

AccountTransactionInfoProtoHelper::AccountTransactionInfoProtoHelper(const keto::proto::AccountTransactionInfo& accountTransactionInfo) {
    this->blockchainId = accountTransactionInfo.blockchain_id();
    this->blockId = accountTransactionInfo.block_id();
    this->transaction = keto::transaction_common::TransactionWrapperHelperPtr(
            new keto::transaction_common::TransactionWrapperHelper(accountTransactionInfo.asn1_transaction_message()));

}

AccountTransactionInfoProtoHelper::~AccountTransactionInfoProtoHelper() {

}


AccountTransactionInfoProtoHelper::operator keto::proto::AccountTransactionInfo() const {
    keto::proto::AccountTransactionInfo accountTransactionInfo;
    accountTransactionInfo.set_blockchain_id(this->blockchainId);
    accountTransactionInfo.set_block_id(this->blockId);
    accountTransactionInfo.set_asn1_transaction_message(this->getTransaction());

    return accountTransactionInfo;
}

AccountTransactionInfoProtoHelper::operator std::string() {
    keto::proto::AccountTransactionInfo accountTransactionInfo;
    accountTransactionInfo.set_blockchain_id(this->blockchainId);
    accountTransactionInfo.set_block_id(this->blockId);
    accountTransactionInfo.set_asn1_transaction_message(this->getTransaction());

    std::string value;
    accountTransactionInfo.SerializeToString(&value);
    return value;
}

AccountTransactionInfoProtoHelper& AccountTransactionInfoProtoHelper::setBlockChainId(const keto::asn1::HashHelper& blockchainId) {
    this->blockchainId = blockchainId;
    return *this;
}

keto::asn1::HashHelper AccountTransactionInfoProtoHelper::getBlockChainId() {
    return this->blockchainId;
}

AccountTransactionInfoProtoHelper& AccountTransactionInfoProtoHelper::setBlockId(const keto::asn1::HashHelper& blockId) {
    this->blockId = blockId;
    return *this;
}

keto::asn1::HashHelper AccountTransactionInfoProtoHelper::getBlockId() {
    return this->blockId;
}

AccountTransactionInfoProtoHelper& AccountTransactionInfoProtoHelper::setTransaction(const keto::transaction_common::TransactionWrapperHelperPtr& transaction) {
    this->transaction = transaction;
    return *this;
}

keto::transaction_common::TransactionWrapperHelperPtr AccountTransactionInfoProtoHelper::getTransaction() {
    return this->transaction;
}

}
}