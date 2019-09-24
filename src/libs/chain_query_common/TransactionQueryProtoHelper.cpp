//
// Created by Brett Chaldecott on 2019-09-21.
//

#include "keto/chain_query_common/TransactionQueryProtoHelper.hpp"

#include "keto/chain_query_common/Constants.hpp"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace chain_query_common {

std::string TransactionQueryProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
};

TransactionQueryProtoHelper::TransactionQueryProtoHelper() {
    this->transactionQuery.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

TransactionQueryProtoHelper::TransactionQueryProtoHelper(const keto::proto::TransactionQuery& transactionQuery) :
    transactionQuery(transactionQuery){

}

TransactionQueryProtoHelper::TransactionQueryProtoHelper(const std::string& msg) {
    this->transactionQuery.ParseFromString(msg);
}

TransactionQueryProtoHelper::~TransactionQueryProtoHelper() {
}

keto::asn1::HashHelper TransactionQueryProtoHelper::getBlockHashId() const {
    return this->transactionQuery.block_hash_id();
}

TransactionQueryProtoHelper& TransactionQueryProtoHelper::setBlockHashId(const keto::asn1::HashHelper& blockHashId) {
    this->transactionQuery.set_block_hash_id(blockHashId);
    return *this;
}

keto::asn1::HashHelper TransactionQueryProtoHelper::getTransactionHashId() const {
    return this->transactionQuery.transaction_hash_id();
}

TransactionQueryProtoHelper& TransactionQueryProtoHelper::setTransactionHashId(const keto::asn1::HashHelper& transactionHashIds) {
    this->transactionQuery.set_transaction_hash_id(transactionHashIds);
    return *this;
}

keto::asn1::HashHelper TransactionQueryProtoHelper::getAccountHashId() const {
    return this->transactionQuery.account_hash_id();
}

TransactionQueryProtoHelper& TransactionQueryProtoHelper::setAccountHashId(const keto::asn1::HashHelper& accountHashIds) {
    this->transactionQuery.set_account_hash_id(accountHashIds);
    return *this;
}

int TransactionQueryProtoHelper::getNumberOfTransactions() const {
    return this->transactionQuery.number_of_transactions();
}

TransactionQueryProtoHelper& TransactionQueryProtoHelper::setNumberOfTransactions(int numberOfTransactions) {
    this->transactionQuery.set_number_of_transactions(numberOfTransactions);
    return *this;
}

TransactionQueryProtoHelper::operator keto::proto::TransactionQuery() const {
    return this->transactionQuery;
}

TransactionQueryProtoHelper::operator std::string() const {
    return this->transactionQuery.SerializeAsString();
}


}
}