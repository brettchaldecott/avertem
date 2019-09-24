//
// Created by Brett Chaldecott on 2019-09-21.
//

#include "keto/chain_query_common/TransactionResultSetProtoHelper.hpp"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace chain_query_common {

std::string TransactionResultSetProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionResultSetProtoHelper::TransactionResultSetProtoHelper() {
    this->transactionResultSet.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

TransactionResultSetProtoHelper::TransactionResultSetProtoHelper(
        const keto::proto::TransactionResultSet& transactionResultSet)  :
    transactionResultSet(transactionResultSet) {

}

TransactionResultSetProtoHelper::TransactionResultSetProtoHelper(const std::string& msg) {
    this->transactionResultSet.ParseFromString(msg);
}
TransactionResultSetProtoHelper::~TransactionResultSetProtoHelper() {

}

std::vector<TransactionResultProtoHelperPtr> TransactionResultSetProtoHelper::getTransactions() {
    std::vector<TransactionResultProtoHelperPtr> result;
    for (int index = 0; index < this->transactionResultSet.transactions_size(); index++) {
        result.push_back(TransactionResultProtoHelperPtr(
                new TransactionResultProtoHelper(this->transactionResultSet.transactions(index))));
    }
    return result;
}

TransactionResultSetProtoHelper& TransactionResultSetProtoHelper::addTransaction(const TransactionResultProtoHelper& transactionResultProtoHelper) {
    *this->transactionResultSet.add_transactions() = transactionResultProtoHelper;
    return *this;
}

TransactionResultSetProtoHelper& TransactionResultSetProtoHelper::addTransaction(const TransactionResultProtoHelperPtr& transactionResultProtoHelperPtr) {
    *this->transactionResultSet.add_transactions() = *transactionResultProtoHelperPtr;
    return *this;
}

int TransactionResultSetProtoHelper::getNumberOfTransactions() {
    return this->transactionResultSet.transactions_size();
}

TransactionResultSetProtoHelper::operator keto::proto::TransactionResultSet() {
    return this->transactionResultSet;
}

TransactionResultSetProtoHelper::operator std::string() {
    return this->transactionResultSet.SerializeAsString();
}

}
}