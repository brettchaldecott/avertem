//
// Created by Brett Chaldecott on 2019-09-21.
//

#include "keto/chain_query_common/TransactionResultProtoHelper.hpp"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace chain_query_common {

std::string TransactionResultProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionResultProtoHelper::TransactionResultProtoHelper() {
    this->transactionResult.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

TransactionResultProtoHelper::TransactionResultProtoHelper(const keto::proto::TransactionResult& transactionResult) :
    transactionResult(transactionResult) {

}

TransactionResultProtoHelper::TransactionResultProtoHelper(const std::string& msg) {
    this->transactionResult.ParseFromString(msg);
}

TransactionResultProtoHelper::~TransactionResultProtoHelper() {

}

keto::asn1::HashHelper TransactionResultProtoHelper::getTransactionHashId() {
    return this->transactionResult.transaction_hash_id();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setTransactionHashId(const keto::asn1::HashHelper& transactionHashId) {
    this->transactionResult.set_transaction_hash_id(transactionHashId);
    return *this;
}

std::time_t TransactionResultProtoHelper::getCreated() {
    return this->transactionResult.created().seconds();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setCreated(const std::time_t& created) {
    google::protobuf::Timestamp createdTime;
    createdTime.set_seconds(created);
    createdTime.set_nanos(0);
    *this->transactionResult.mutable_created() = createdTime;
    return *this;
}

keto::asn1::HashHelper TransactionResultProtoHelper::getParentTransactionHashId() {
    return this->transactionResult.parent_transaction_hash_id();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setParentTransactionHashId(const keto::asn1::HashHelper& transactionHashId) {
    this->transactionResult.set_parent_transaction_hash_id(transactionHashId);
    return *this;
}

keto::asn1::HashHelper TransactionResultProtoHelper::getSourceAccountHashId() {
    return this->transactionResult.source_account_hash_id();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setSourceAccountHashId(const keto::asn1::HashHelper& sourceAccountHashId) {
    this->transactionResult.set_transaction_hash_id(sourceAccountHashId);
    return *this;
}

keto::asn1::HashHelper TransactionResultProtoHelper::getTargetAccountHashId() {
    return this->transactionResult.target_account_hash_id();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setTargetAccountHashId(const keto::asn1::HashHelper& targetAccountHashId) {
    this->transactionResult.set_transaction_hash_id(targetAccountHashId);
    return *this;
}

keto::asn1::NumberHelper TransactionResultProtoHelper::getValue() {
    return this->transactionResult.value();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setValue(const keto::asn1::NumberHelper& value) {
    this->transactionResult.set_value(value);
    return *this;
}

keto::asn1::SignatureHelper TransactionResultProtoHelper::getSignature() {
    return this->transactionResult.signature();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setSignature(const keto::asn1::SignatureHelper& signature) {
    this->transactionResult.set_signature(signature);
    return *this;
}

std::vector<keto::asn1::HashHelper> TransactionResultProtoHelper::getChangesetHashes() {
    std::vector<keto::asn1::HashHelper> changesetHashes;
    for (int index = 0; index < this->transactionResult.changeset_hashes_size(); index++){
        changesetHashes.push_back(keto::asn1::HashHelper(this->transactionResult.changeset_hashes(index)));
    }
    return changesetHashes;
}

TransactionResultProtoHelper& TransactionResultProtoHelper::addChangesetHash(const keto::asn1::HashHelper& changeSetHash) {
    this->transactionResult.add_changeset_hashes(changeSetHash);
    return *this;
}

std::vector<keto::asn1::HashHelper> TransactionResultProtoHelper::getTransactionTraceHashes() {
    std::vector<keto::asn1::HashHelper> transactionTraceHashes;
    for (int index = 0; index < this->transactionResult.transaction_trace_hashes_size(); index++){
        transactionTraceHashes.push_back(keto::asn1::HashHelper(this->transactionResult.transaction_trace_hashes(index)));
    }
    return transactionTraceHashes;
}

TransactionResultProtoHelper& TransactionResultProtoHelper::addTransactionTraceHash(const keto::asn1::HashHelper& transactionHash) {
    this->transactionResult.add_transaction_trace_hashes(transactionHash);
    return *this;
}

int TransactionResultProtoHelper::getStatus() {
    return this->transactionResult.status();
}

TransactionResultProtoHelper& TransactionResultProtoHelper::setStatus(int status) {
    this->transactionResult.set_status(status);
    return *this;
}

TransactionResultProtoHelper::operator keto::proto::TransactionResult() const {
    return this->transactionResult;
}

TransactionResultProtoHelper::operator std::string() const {
    return this->transactionResult.SerializeAsString();
}

}
}