/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MessageWrapperProtoHelper.cpp
 * Author: ubuntu
 * 
 * Created on May 21, 2018, 2:32 PM
 */

#include "keto/asn1/HashHelper.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"
#include "include/keto/transaction_common/MessageWrapperProtoHelper.hpp"


namespace keto {
namespace transaction_common {

std::string MessageWrapperProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

MessageWrapperProtoHelper::MessageWrapperProtoHelper() {
    wrapper.set_version(1);
    wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_INIT);
    wrapper.set_message_type(keto::proto::MessageType::MESSAGE_TYPE_TRANSACTION);
    
}

MessageWrapperProtoHelper::MessageWrapperProtoHelper(const keto::proto::MessageWrapper& wrapper) : wrapper(wrapper) {
}

MessageWrapperProtoHelper::MessageWrapperProtoHelper(const std::string& str) {
    wrapper.ParseFromString(str);
}

MessageWrapperProtoHelper::MessageWrapperProtoHelper(const keto::proto::Transaction& transaction) {
    wrapper.set_version(1);
    wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_INIT);
    wrapper.set_message_type(keto::proto::MessageType::MESSAGE_TYPE_TRANSACTION);
    
    TransactionProtoHelper transactionProtoHelper(transaction);
    setTransaction(transactionProtoHelper);
}

MessageWrapperProtoHelper::MessageWrapperProtoHelper(const TransactionProtoHelper& transaction) {
    wrapper.set_version(1);
    wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_INIT);
    wrapper.set_message_type(keto::proto::MessageType::MESSAGE_TYPE_TRANSACTION);
    
    setTransaction((TransactionProtoHelper&)transaction);
}

MessageWrapperProtoHelper::MessageWrapperProtoHelper(const TransactionProtoHelperPtr& transaction) {
    wrapper.set_version(1);
    wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_INIT);
    wrapper.set_message_type(keto::proto::MessageType::MESSAGE_TYPE_TRANSACTION);
    
    setTransaction((TransactionProtoHelper&)*transaction);
}

MessageWrapperProtoHelper::~MessageWrapperProtoHelper() {
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setAccountHash(const keto::asn1::HashHelper accountHash) {
    wrapper.set_account_hash(accountHash);
    return *this;
}

keto::asn1::HashHelper MessageWrapperProtoHelper::getAccountHash() {
    return keto::asn1::HashHelper(wrapper.account_hash());
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setSessionHash(const keto::asn1::HashHelper sessionHash) {
    wrapper.set_session_hash(sessionHash);
    return *this;
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setSessionHash(const std::string& sessionHash) {
    wrapper.set_session_hash(sessionHash);
    return *this;
}
    
keto::asn1::HashHelper MessageWrapperProtoHelper::getSessionHash() {
    return keto::asn1::HashHelper(wrapper.session_hash());
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setOperation(const keto::proto::MessageOperation operation) {
    this->wrapper.set_message_operation(operation);
    return *this;
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setMessageWrapper(const keto::proto::MessageWrapper& wrapper) {
    this->wrapper = wrapper;
    return *this;
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setTransaction(const keto::proto::Transaction& transaction) {
    TransactionProtoHelper transactionProtoHelper(transaction);
    setTransaction(transactionProtoHelper);
    return *this;
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setTransaction(TransactionProtoHelper& transaction) {
    keto::asn1::HashHelper accountHash = 
            transaction.getTransactionMessageHelper()->getTransactionWrapper()->getCurrentAccount();
    std::vector<uint8_t> accountVectorHash = keto::crypto::SecureVectorUtils().copyFromSecure(accountHash);
    wrapper.set_account_hash(accountVectorHash.data(),accountVectorHash.size());
    google::protobuf::Any* any = new google::protobuf::Any();
    keto::proto::Transaction transactionProto = transaction;
    any->PackFrom(transactionProto);
    wrapper.set_allocated_msg(any);
    return *this;
}

MessageWrapperProtoHelper& MessageWrapperProtoHelper::setTransaction(const TransactionProtoHelperPtr& transaction) {
    setTransaction((TransactionProtoHelper&)*transaction);
    return *this;    
}


keto::proto::MessageOperation MessageWrapperProtoHelper::incrementOperation() {
    if (this->wrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_INIT) {
        this->wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_ROUTE);
    } else if (this->wrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
        this->wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BALANCE);
    } else if (this->wrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_BALANCE) {
        this->wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BLOCK);
    } else if (this->wrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_BLOCK) {
        this->wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_PROCESS);
    } else if (this->wrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_PROCESS) {
        this->wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_WAIT);
    } else if (this->wrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_WAIT) {
        this->wrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_PERSIST);
    }
    return this->wrapper.message_operation();
}


TransactionProtoHelperPtr MessageWrapperProtoHelper::getTransaction() {
    keto::proto::Transaction transaction;
    this->wrapper.msg().UnpackTo(&transaction);
    return TransactionProtoHelperPtr(new TransactionProtoHelper(transaction));
}

MessageWrapperProtoHelper::operator keto::proto::MessageWrapper() {
    return this->wrapper;
}


}
}
