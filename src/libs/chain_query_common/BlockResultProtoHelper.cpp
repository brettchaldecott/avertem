//
// Created by Brett Chaldecott on 2019-09-20.
//

#include "keto/chain_query_common/BlockResultProtoHelper.hpp"

#include "keto/chain_query_common/Constants.hpp"

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace chain_query_common {

std::string BlockResultProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockResultProtoHelper::BlockResultProtoHelper() {
    this->blockResult.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

BlockResultProtoHelper::BlockResultProtoHelper(const keto::proto::BlockResult& blockResult) : blockResult(blockResult){

}

BlockResultProtoHelper::BlockResultProtoHelper(const std::string& msg) {
    this->blockResult.ParseFromString(msg);
}

BlockResultProtoHelper::~BlockResultProtoHelper() {

}

keto::asn1::HashHelper BlockResultProtoHelper::getTangleHashId() {
    return this->blockResult.tangle_hash_id();
}

BlockResultProtoHelper& BlockResultProtoHelper::setTangleHashId(const keto::asn1::HashHelper& hashHelper) {
    this->blockResult.set_tangle_hash_id(hashHelper);
    return *this;
}

keto::asn1::HashHelper BlockResultProtoHelper::getBlockHashId() {
    return this->blockResult.block_hash_id();
}

BlockResultProtoHelper& BlockResultProtoHelper::setBlockHashId(const keto::asn1::HashHelper& hashHelper) {
    this->blockResult.set_block_hash_id(hashHelper);
    return *this;
}

std::time_t BlockResultProtoHelper::getCreated() {
    return this->blockResult.created().seconds();
}

BlockResultProtoHelper& BlockResultProtoHelper::setCreated(const std::time_t& created) {
    google::protobuf::Timestamp createdTime;
    createdTime.set_seconds(created);
    createdTime.set_nanos(0);
    *this->blockResult.mutable_created() = createdTime;
    return *this;
}

keto::asn1::HashHelper BlockResultProtoHelper::getParentBlockHashId() {
    return this->blockResult.parent_block_hash_id();
}


BlockResultProtoHelper& BlockResultProtoHelper::setParentBlockHashId(const keto::asn1::HashHelper& parentBlockHashId) {
    this->blockResult.set_parent_block_hash_id(parentBlockHashId);
    return *this;
}

std::vector<TransactionResultProtoHelperPtr> BlockResultProtoHelper::getTransactions() {
    std::vector<TransactionResultProtoHelperPtr> transactions;
    for (int index = 0; index < this->blockResult.transactions_size(); index++) {
        transactions.push_back(TransactionResultProtoHelperPtr(
                new TransactionResultProtoHelper(this->blockResult.transactions(index))));
    }
    return transactions;
}

BlockResultProtoHelper& BlockResultProtoHelper::addTransaction(const TransactionResultProtoHelper& transactionResultProtoHelper) {
    *this->blockResult.add_transactions() = transactionResultProtoHelper;
    return *this;
}

BlockResultProtoHelper& BlockResultProtoHelper::addTransaction(const TransactionResultProtoHelperPtr& transactionResultProtoHelperPtr) {
    *this->blockResult.add_transactions() = *transactionResultProtoHelperPtr;
    return *this;
}


keto::asn1::HashHelper BlockResultProtoHelper::getMerkelRoot() {
    return this->blockResult.merkel_root();
}

BlockResultProtoHelper& BlockResultProtoHelper::setMerkelRoot(const keto::asn1::HashHelper& merkelRoot) {
    this->blockResult.set_merkel_root(merkelRoot);
    return *this;
}

keto::asn1::HashHelper BlockResultProtoHelper::getAcceptedHash() {
    return this->blockResult.accepted_hash();
}

BlockResultProtoHelper& BlockResultProtoHelper::setAcceptedHash(const keto::asn1::HashHelper& acceptedHash) {
    this->blockResult.set_accepted_hash(acceptedHash);
    return *this;
}

keto::asn1::HashHelper BlockResultProtoHelper::getValidationHash() {
    return this->blockResult.validation_hash();
}

BlockResultProtoHelper& BlockResultProtoHelper::setValidationHash(const keto::asn1::HashHelper& validationHash) {
    this->blockResult.set_validation_hash(validationHash);
    return *this;
}

keto::asn1::SignatureHelper BlockResultProtoHelper::getSignature() {
    return this->blockResult.signature();
}

BlockResultProtoHelper& BlockResultProtoHelper::setSignature(const keto::asn1::SignatureHelper& signature) {
    this->blockResult.set_signature(signature);
    return *this;
}

int BlockResultProtoHelper::getBlockHeight() {
    return this->blockResult.block_height();
}

BlockResultProtoHelper& BlockResultProtoHelper::setBlockHeight(int blockHeight) {
    this->blockResult.set_block_height(blockHeight);
    return *this;
}

BlockResultProtoHelper::operator keto::proto::BlockResult() const {
    return this->blockResult;
}

BlockResultProtoHelper::operator std::string() const {
    return this->blockResult.SerializeAsString();
}

}
}
