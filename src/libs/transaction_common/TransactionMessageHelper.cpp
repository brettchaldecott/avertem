/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionMessageHelper.cpp
 * Author: ubuntu
 * 
 * Created on March 17, 2018, 4:21 AM
 */

#include "asn_SEQUENCE_OF.h"
#include "EncryptedDataWrapper.h"

#include <botan/hex.h>

#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/transaction_common/Exception.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/DeserializationHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"

namespace keto {
namespace transaction_common {

std::string TransactionMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
TransactionMessageHelper::TransactionMessageHelper() {
    this->transactionMessage = (TransactionMessage_t*)calloc(1, sizeof *transactionMessage);
    this->transactionMessage->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionMessage->availableTime = 0;
    this->transactionMessage->elapsedTime = 0;
}


TransactionMessageHelper::TransactionMessageHelper(const TransactionWrapperHelperPtr& transactionWrapper) {
    this->transactionMessage = (TransactionMessage_t*)calloc(1, sizeof *transactionMessage);
    this->transactionMessage->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionMessage->availableTime = 0;
    this->transactionMessage->elapsedTime = 0;
    this->transactionMessage->transaction = *transactionWrapper->operator TransactionWrapper_t*();
}

TransactionMessageHelper::TransactionMessageHelper(TransactionWrapper_t* transactionWrapper) {
    this->transactionMessage = (TransactionMessage_t*)calloc(1, sizeof *transactionMessage);
    this->transactionMessage->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionMessage->availableTime = 0;
    this->transactionMessage->elapsedTime = 0;
    this->transactionMessage->transaction = *transactionWrapper;
}


TransactionMessageHelper::TransactionMessageHelper(TransactionMessage_t* transactionMessage) :
    transactionMessage(transactionMessage) {
}

TransactionMessageHelper::TransactionMessageHelper(const TransactionMessage_t& transactionMessage) {
    this->transactionMessage = (TransactionMessage_t*)calloc(1, sizeof *this->transactionMessage);
    this->transactionMessage->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionMessage->availableTime = transactionMessage.availableTime;
    this->transactionMessage->elapsedTime = transactionMessage.elapsedTime;
}

TransactionMessageHelper::TransactionMessageHelper(const std::string& transactionMessage) {
    this->transactionMessage =
            keto::asn1::DeserializationHelper<TransactionMessage_t>((const uint8_t*)transactionMessage.data(), 
            transactionMessage.size(),&asn_DEF_TransactionMessage).takePtr();
}

TransactionMessageHelper::TransactionMessageHelper(const TransactionMessageHelper& orig) {
    this->transactionMessage = keto::asn1::clone<TransactionMessage_t>(orig.transactionMessage,&asn_DEF_TransactionMessage);
}


TransactionMessageHelper::~TransactionMessageHelper() {
    if (transactionMessage) {
        ASN_STRUCT_FREE(asn_DEF_TransactionMessage, transactionMessage);
        transactionMessage = 0;
    }
}

TransactionMessageHelper& TransactionMessageHelper::setTransactionWrapper(
        TransactionWrapper_t* transactionWrapper) {
    this->transactionMessage->transaction = *transactionWrapper;
    return *this;
}

TransactionMessageHelper& TransactionMessageHelper::setTransactionWrapper(const TransactionWrapperHelperPtr& transactionWrapper) {
    TransactionWrapper_t* transactionWrapper1 = (TransactionWrapper_t*)calloc(1, sizeof *transactionWrapper1);
    *transactionWrapper1 = this->transactionMessage->transaction;
    ASN_STRUCT_FREE(asn_DEF_TransactionWrapper, transactionWrapper1);
    this->transactionMessage->transaction = *(transactionWrapper->operator TransactionWrapper_t*());
    return *this;
}

TransactionWrapperHelperPtr TransactionMessageHelper::getTransactionWrapper() {
    return TransactionWrapperHelperPtr(
            new TransactionWrapperHelper(&this->transactionMessage->transaction,false));
}

TransactionMessageHelper& TransactionMessageHelper::addNestedTransaction(TransactionMessage_t* nestedTransaction) {
    ASN_SEQUENCE_ADD(&transactionMessage->sideTransactions,nestedTransaction);
    return *this;
}


TransactionMessageHelper& TransactionMessageHelper::addNestedTransaction(
        const TransactionMessageHelperPtr& nestedTransaction) {
    ASN_SEQUENCE_ADD(&transactionMessage->sideTransactions,(TransactionMessage_t*)*nestedTransaction);

    return *this;
}
TransactionMessageHelper& TransactionMessageHelper::addNestedTransaction(
        const TransactionMessageHelper& nestedTransaction) {

    ASN_SEQUENCE_ADD(&transactionMessage->sideTransactions,(TransactionMessage_t*)nestedTransaction);

    return *this;
}

std::vector<TransactionMessageHelperPtr> TransactionMessageHelper::getNestedTransactions() {
    std::vector<TransactionMessageHelperPtr> nestedTransactions;
    for (int index = 0; index < this->transactionMessage->sideTransactions.list.count; index++) {
        nestedTransactions.push_back(TransactionMessageHelperPtr(new TransactionMessageHelper(
                keto::asn1::clone<TransactionMessage_t>(this->transactionMessage->sideTransactions.list.array[index],&asn_DEF_TransactionMessage))));
    }
    return nestedTransactions;
}

TransactionMessageHelper& TransactionMessageHelper::operator =(const std::string& transactionMessage) {
    this->transactionMessage = 
            keto::asn1::DeserializationHelper<TransactionMessage_t>((const uint8_t*)transactionMessage.data(), 
            transactionMessage.size(),&asn_DEF_TransactionMessage).takePtr();
    return (*this);
}

TransactionMessageHelper::operator TransactionMessage_t&() {
    return (*this->transactionMessage);
}

TransactionMessageHelper::operator TransactionMessage_t*() const {
    return this->getMessage();
}

TransactionMessageHelper::operator ANY_t*() {
    TransactionMessage_t* transactionMessage = getMessage();
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_TransactionMessage, transactionMessage);
    if (!anyPtr) {
        ASN_STRUCT_FREE(asn_DEF_TransactionMessage, transactionMessage);
        BOOST_THROW_EXCEPTION(keto::transaction_common::ANYSerializationFailedException());
    }
    ASN_STRUCT_FREE(asn_DEF_TransactionMessage, transactionMessage);
    return anyPtr;
}

TransactionMessageHelper::operator std::vector<uint8_t>() {
    return keto::asn1::SerializationHelper<TransactionMessage>(this->transactionMessage,
        &asn_DEF_TransactionMessage);
}

std::vector<uint8_t> TransactionMessageHelper::serializeTransaction(TransactionEncryptionHandler& 
            transactionEncryptionHandler) {
    TransactionMessage_t* transactionMessage = getMessage(transactionEncryptionHandler);
    
    std::vector<uint8_t> bytes = keto::asn1::SerializationHelper<TransactionMessage>(transactionMessage,
        &asn_DEF_TransactionMessage).operator std::vector<uint8_t>&();
    
    ASN_STRUCT_FREE(asn_DEF_TransactionMessage, transactionMessage);
    
    return bytes;
}

TransactionMessage_t* TransactionMessageHelper::getMessage() const {
    return keto::asn1::clone<TransactionMessage_t>(this->transactionMessage,&asn_DEF_TransactionMessage);
}

TransactionMessage_t* TransactionMessageHelper::getMessage(TransactionEncryptionHandler& 
        transactionEncryptionHandler) {
    TransactionMessage_t* _transactionMessage =  cloneTransaction(this->transactionMessage);
    for (int index = 0; index < this->transactionMessage->sideTransactions.list.count; index++) {
        TransactionMessageHelperPtr transactionMessageHelperPtr(new TransactionMessageHelper(
                keto::asn1::clone<TransactionMessage_t>(this->transactionMessage->sideTransactions.list.array[index],&asn_DEF_TransactionMessage)));
        TransactionMessage_t* childTransaction = transactionMessageHelperPtr->getMessage(transactionEncryptionHandler);
        if (transactionMessageHelperPtr->isEncrypted()) {
            EncryptedDataWrapper_t* encryptedDataWrapper = transactionEncryptionHandler.encrypt(*childTransaction);
            ASN_SEQUENCE_ADD(&_transactionMessage->encryptedSideTransactions,encryptedDataWrapper);
            ASN_STRUCT_FREE(asn_DEF_TransactionMessage, childTransaction);
        } else {
            ASN_SEQUENCE_ADD(&_transactionMessage->sideTransactions,childTransaction);
        }
    }
    return _transactionMessage;
}

TransactionMessageHelperPtr TransactionMessageHelper::decryptMessage(TransactionEncryptionHandler& transactionEncryptionHandler) {
    TransactionMessageHelperPtr transactionMessageHelperPtr(new TransactionMessageHelper(*this->transactionMessage));
    transactionMessageHelperPtr->setTransactionWrapper(keto::asn1::clone<TransactionWrapper_t>(
            &this->transactionMessage->transaction,&asn_DEF_TransactionWrapper));
    
    for (int index = 0; index < this->transactionMessage->encryptedSideTransactions.list.count; index++) {
        decryptMessage(transactionEncryptionHandler,transactionMessageHelperPtr,
                this->transactionMessage->encryptedSideTransactions.list.array[index]);
    }
    for (int index = 0; index < this->transactionMessage->sideTransactions.list.count; index++) {
        decryptMessage(transactionEncryptionHandler,transactionMessageHelperPtr,
                       keto::asn1::clone<TransactionMessage_t>(
                               this->transactionMessage->sideTransactions.list.array[index],&asn_DEF_TransactionMessage));
    }
    return transactionMessageHelperPtr;
}

bool TransactionMessageHelper::isEncrypted() {
    return this->transactionMessage->transaction.signedTransaction.transaction.encrypted;
}

time_t TransactionMessageHelper::getAvailableTime() {
    return this->transactionMessage->availableTime;
}

TransactionMessageHelper& TransactionMessageHelper::setAvailableTime(time_t availableTime) {
    this->transactionMessage->availableTime = availableTime;
    return *this;
}

time_t TransactionMessageHelper::getElapsedTime() {
    return this->transactionMessage->elapsedTime;
}

TransactionMessageHelper& TransactionMessageHelper::setElapsedTime(time_t elapsedTime) {
    this->transactionMessage->elapsedTime = elapsedTime;
    return *this;
}

void TransactionMessageHelper::decryptMessage(TransactionEncryptionHandler& transactionEncryptionHandler,
        TransactionMessageHelperPtr transactionMessageHelperPtr,
        EncryptedDataWrapper_t* encryptedDataWrapper) {
    decryptMessage(transactionEncryptionHandler, transactionMessageHelperPtr, transactionEncryptionHandler.decrypt(*encryptedDataWrapper));
}

void TransactionMessageHelper::decryptMessage(TransactionEncryptionHandler& transactionEncryptionHandler,
        TransactionMessageHelperPtr transactionMessageHelperPtr,
        TransactionMessage_t* transactionMessage) {

    TransactionMessageHelperPtr _transactionMessageHelperPtr(new TransactionMessageHelper(
            *transactionMessage));
    for (int index = 0; index < transactionMessage->encryptedSideTransactions.list.count; index++) {
        decryptMessage(transactionEncryptionHandler, _transactionMessageHelperPtr,
                transactionMessage->encryptedSideTransactions.list.array[index]);
    }
    for (int index = 0; index < transactionMessage->sideTransactions.list.count; index++) {
        decryptMessage(transactionEncryptionHandler, _transactionMessageHelperPtr,
                       keto::asn1::clone<TransactionMessage_t>(
                               transactionMessage->sideTransactions.list.array[index],&asn_DEF_TransactionMessage));
    }
    transactionMessageHelperPtr->addNestedTransaction(_transactionMessageHelperPtr);
    ASN_STRUCT_FREE(asn_DEF_TransactionMessage, transactionMessage);
}


TransactionMessage_t* TransactionMessageHelper::cloneTransaction(TransactionMessage_t* source) {
    TransactionMessage_t* _transactionMessage =
            keto::asn1::clone<TransactionMessage_t>(source,&asn_DEF_TransactionMessage);
    while (_transactionMessage->sideTransactions.list.count) {
        asn_sequence_del(&_transactionMessage->sideTransactions,0,true);
    }
    while (_transactionMessage->encryptedSideTransactions.list.count) {
        asn_sequence_del(&_transactionMessage->encryptedSideTransactions,0,true);
    }
    return _transactionMessage;
}


}
}
