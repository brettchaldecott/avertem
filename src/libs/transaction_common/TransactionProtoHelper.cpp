/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionProtoHelper.cpp
 * Author: ubuntu
 * 
 * Created on March 19, 2018, 7:59 AM
 */

#include "keto/transaction_common/TransactionProtoHelper.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/SignatureGenerator.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

#include "include/keto/transaction_common/TransactionProtoHelper.hpp"
#include "include/keto/transaction_common/TransactionMessageHelper.hpp"


namespace keto {
namespace transaction_common {

std::string TransactionProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionProtoHelper::TransactionProtoHelper() {
}

TransactionProtoHelper::TransactionProtoHelper(const keto::proto::Transaction& transaction) {
    this->transaction.CopyFrom(transaction);
}


TransactionProtoHelper::TransactionProtoHelper(
        const TransactionMessageHelperPtr& transactionMessageHelper) {
    TransactionWrapperHelperPtr transactionWrapperHelperPtr = 
            transactionMessageHelper->getTransactionWrapper();
    keto::asn1::HashHelper hashHelper = transactionWrapperHelperPtr->getHash();
    transaction.set_transaction_hash(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    keto::asn1::SignatureHelper signatureHelper = transactionWrapperHelperPtr->getSignature();
    transaction.set_transaction_signature(
        signatureHelper.operator std::vector<uint8_t>().data(),
        signatureHelper.operator std::vector<uint8_t>().size());
    hashHelper = transactionWrapperHelperPtr->getCurrentAccount();
    transaction.set_active_account(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    
    if (transactionWrapperHelperPtr->getStatus() == Status_init) {
        transaction.set_status(keto::proto::TransactionStatus::INIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_debit) {
        transaction.set_status(keto::proto::TransactionStatus::DEBIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_processing) {
        transaction.set_status(keto::proto::TransactionStatus::PROCESS);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_credit) {
        transaction.set_status(keto::proto::TransactionStatus::CREDIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_complete) {
        transaction.set_status(keto::proto::TransactionStatus::COMPLETE);
    }
    
    std::vector<uint8_t> serializedTransaction = 
        *transactionMessageHelper;
    transaction.set_asn1_transaction_message(
        serializedTransaction.data(),serializedTransaction.size());
}


TransactionProtoHelper::TransactionProtoHelper(const TransactionMessageHelperPtr& transactionMessageHelper,
        keto::transaction_common::TransactionEncryptionHandler& transactionEncryptionHandler) {
    TransactionWrapperHelperPtr transactionWrapperHelperPtr = 
            transactionMessageHelper->getTransactionWrapper();
    keto::asn1::HashHelper hashHelper = transactionWrapperHelperPtr->getHash();
    transaction.set_transaction_hash(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    keto::asn1::SignatureHelper signatureHelper = transactionWrapperHelperPtr->getSignature();
    transaction.set_transaction_signature(
        signatureHelper.operator std::vector<uint8_t>().data(),
        signatureHelper.operator std::vector<uint8_t>().size());
    hashHelper = transactionWrapperHelperPtr->getCurrentAccount();
    transaction.set_active_account(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    
    if (transactionWrapperHelperPtr->getStatus() == Status_init) {
        transaction.set_status(keto::proto::TransactionStatus::INIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_debit) {
        transaction.set_status(keto::proto::TransactionStatus::DEBIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_processing) {
        transaction.set_status(keto::proto::TransactionStatus::PROCESS);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_credit) {
        transaction.set_status(keto::proto::TransactionStatus::CREDIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_complete) {
        transaction.set_status(keto::proto::TransactionStatus::COMPLETE);
    }
    
    std::vector<uint8_t> serializedTransaction = 
        transactionMessageHelper->serializeTransaction(transactionEncryptionHandler);
    
    transaction.set_asn1_transaction_message(
        serializedTransaction.data(),serializedTransaction.size());
}


TransactionProtoHelper::~TransactionProtoHelper() {
}

TransactionProtoHelper& TransactionProtoHelper::setTransaction(
    const TransactionMessageHelperPtr& transactionMessageHelper) {
    TransactionWrapperHelperPtr transactionWrapperHelperPtr = 
            transactionMessageHelper->getTransactionWrapper();
    keto::asn1::HashHelper hashHelper = transactionWrapperHelperPtr->getHash();
    transaction.set_transaction_hash(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    keto::asn1::SignatureHelper signatureHelper = transactionWrapperHelperPtr->getSignature();
    transaction.set_transaction_signature(
        signatureHelper.operator std::vector<uint8_t>().data(),
        signatureHelper.operator std::vector<uint8_t>().size());
    hashHelper = transactionWrapperHelperPtr->getCurrentAccount();
    transaction.set_active_account(
        hashHelper.operator keto::crypto::SecureVector().data(),
        hashHelper.operator keto::crypto::SecureVector().size());
    
    
    if (transactionWrapperHelperPtr->getStatus() == Status_init) {
        transaction.set_status(keto::proto::TransactionStatus::INIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_debit) {
        transaction.set_status(keto::proto::TransactionStatus::DEBIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_processing) {
        transaction.set_status(keto::proto::TransactionStatus::PROCESS);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_nested) {
        transaction.set_status(keto::proto::TransactionStatus::NESTED);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_credit) {
        transaction.set_status(keto::proto::TransactionStatus::CREDIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_complete) {
        transaction.set_status(keto::proto::TransactionStatus::COMPLETE);
    }
    
    std::vector<uint8_t> serializedTransaction = *transactionMessageHelper;
    transaction.set_asn1_transaction_message(
        serializedTransaction.data(),serializedTransaction.size());
    
    return (*this);
}

TransactionProtoHelper& TransactionProtoHelper::setTransaction(
        const TransactionMessageHelperPtr& transactionMessageHelper,
        keto::transaction_common::TransactionEncryptionHandler& transactionEncryptionHandler) {
    TransactionWrapperHelperPtr transactionWrapperHelperPtr =
            transactionMessageHelper->getTransactionWrapper();
    keto::asn1::HashHelper hashHelper = transactionWrapperHelperPtr->getHash();
    transaction.set_transaction_hash(
            hashHelper.operator keto::crypto::SecureVector().data(),
            hashHelper.operator keto::crypto::SecureVector().size());
    keto::asn1::SignatureHelper signatureHelper = transactionWrapperHelperPtr->getSignature();
    transaction.set_transaction_signature(
            signatureHelper.operator std::vector<uint8_t>().data(),
            signatureHelper.operator std::vector<uint8_t>().size());
    hashHelper = transactionWrapperHelperPtr->getCurrentAccount();
    transaction.set_active_account(
            hashHelper.operator keto::crypto::SecureVector().data(),
            hashHelper.operator keto::crypto::SecureVector().size());


    if (transactionWrapperHelperPtr->getStatus() == Status_init) {
        transaction.set_status(keto::proto::TransactionStatus::INIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_debit) {
        transaction.set_status(keto::proto::TransactionStatus::DEBIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_processing) {
        transaction.set_status(keto::proto::TransactionStatus::PROCESS);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_nested) {
        transaction.set_status(keto::proto::TransactionStatus::NESTED);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_credit) {
        transaction.set_status(keto::proto::TransactionStatus::CREDIT);
    } else if (transactionWrapperHelperPtr->getStatus() == Status_complete) {
        transaction.set_status(keto::proto::TransactionStatus::COMPLETE);
    }

    std::vector<uint8_t> serializedTransaction =
            transactionMessageHelper->serializeTransaction(transactionEncryptionHandler);

    transaction.set_asn1_transaction_message(
            serializedTransaction.data(),serializedTransaction.size());

    return (*this);
}

TransactionProtoHelper& TransactionProtoHelper::setTransaction(
        const std::string& buffer) {
    transaction.ParseFromString(buffer);
    return (*this);
}

keto::asn1::HashHelper TransactionProtoHelper::getActiveAccount() {
    return this->transaction.active_account();
}

TransactionProtoHelper::operator std::string() const {
    std::string buffer;
    transaction.SerializeToString(&buffer);
    return buffer;
}

TransactionProtoHelper::operator keto::proto::Transaction() const {
    return this->transaction;
}


TransactionProtoHelper& TransactionProtoHelper::operator = (const keto::proto::Transaction& transaction) {
    this->transaction.CopyFrom(transaction);
    return (*this);
}

TransactionMessageHelperPtr TransactionProtoHelper::getTransactionMessageHelper() {
    return TransactionMessageHelperPtr(
            new TransactionMessageHelper(this->transaction.asn1_transaction_message()));
}

    
}
}