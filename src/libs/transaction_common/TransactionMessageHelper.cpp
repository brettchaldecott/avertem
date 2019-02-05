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
}


TransactionMessageHelper::TransactionMessageHelper(const TransactionWrapperHelperPtr& transactionWrapper) {
    this->transactionMessage = (TransactionMessage_t*)calloc(1, sizeof *transactionMessage);
    this->transactionMessage->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionMessage->transaction = *transactionWrapper->operator TransactionWrapper_t*();
}

TransactionMessageHelper::TransactionMessageHelper(TransactionWrapper_t* transactionWrapper) {
    this->transactionMessage = (TransactionMessage_t*)calloc(1, sizeof *transactionMessage);
    this->transactionMessage->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionMessage->transaction = *transactionWrapper;
}


TransactionMessageHelper::TransactionMessageHelper(TransactionMessage_t* transactionMessage) :
    transactionMessage(transactionMessage) {
}

TransactionMessageHelper::TransactionMessageHelper(const std::string& transactionMessage) {
    this->transactionMessage = 
            keto::asn1::DeserializationHelper<TransactionMessage_t>((const uint8_t*)transactionMessage.data(), 
            transactionMessage.size(),&asn_DEF_TransactionMessage).takePtr();
}

TransactionMessageHelper::TransactionMessageHelper(const TransactionMessageHelper& orig) {
    this->transactionMessage = keto::asn1::clone<TransactionMessage_t>(orig.transactionMessage,&asn_DEF_TransactionMessage);
    this->encrypt = orig.encrypt;
    this->nestedTransactions = orig.nestedTransactions;
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
    this->transactionMessage->transaction = *(transactionWrapper->operator TransactionWrapper_t*());
    return *this;
}

TransactionWrapperHelperPtr TransactionMessageHelper::getTransactionWrapper() {
    return TransactionWrapperHelperPtr(
            new TransactionWrapperHelper(&this->transactionMessage->transaction,false));
}

TransactionMessageHelper& TransactionMessageHelper::setEncrypted(bool encrypted) {
    this->encrypt = encrypted;
    return *this;
}

TransactionMessageHelper& TransactionMessageHelper::addNestedTransaction(TransactionMessage_t* nestedTransaction) {
    this->nestedTransactions.push_back(TransactionMessageHelperPtr(
            new TransactionMessageHelper(nestedTransaction)));
    return *this;
}


TransactionMessageHelper& TransactionMessageHelper::addNestedTransaction(
        const TransactionMessageHelperPtr& nestedTransaction) {
    this->nestedTransactions.push_back(nestedTransaction);
    return *this;
}
TransactionMessageHelper& TransactionMessageHelper::addNestedTransaction(
        const TransactionMessageHelper& nestedTransaction) {
    this->nestedTransactions.push_back(TransactionMessageHelperPtr(
            new TransactionMessageHelper(nestedTransaction)));
    return *this;
}

int TransactionMessageHelper::numberOfNestedTransactions() {
    return this->nestedTransactions.size();
}

TransactionMessageHelperPtr TransactionMessageHelper::getNestedTransaction(int index) {
    return nestedTransactions[index];
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

TransactionMessageHelper::operator TransactionMessage_t*() {
    TransactionMessage_t* result = this->transactionMessage;
    this->transactionMessage = 0;
    return result;
}

TransactionMessageHelper::operator ANY_t*() {
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_TransactionMessage, this->transactionMessage);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::ANYSerializationFailedException());
    }
    return anyPtr;
}

TransactionMessageHelper::operator std::vector<uint8_t>() {
    return keto::asn1::SerializationHelper<TransactionMessage>(this->transactionMessage,
        &asn_DEF_TransactionMessage).operator std::vector<uint8_t>&();
}

std::vector<uint8_t> TransactionMessageHelper::serializeTransaction(TransactionEncryptionHandler& 
            transactionEncryptionHandler) {
    TransactionMessage_t* transactionMessage = getMessage(transactionEncryptionHandler);
    
    std::vector<uint8_t> bytes = keto::asn1::SerializationHelper<TransactionMessage>(transactionMessage,
        &asn_DEF_TransactionMessage).operator std::vector<uint8_t>&();
    
    ASN_STRUCT_FREE(asn_DEF_TransactionMessage, transactionMessage);
    
    return bytes;
}
    
TransactionMessage_t* TransactionMessageHelper::getMessage(TransactionEncryptionHandler& 
        transactionEncryptionHandler) {
    TransactionMessage_t* transactionMessage = 
            keto::asn1::clone<TransactionMessage_t>(this->transactionMessage,&asn_DEF_TransactionMessage);
    for (TransactionMessageHelperPtr transactionMessageHelperPtr : nestedTransactions) {
        transactionMessageHelperPtr->getMessage(transactionEncryptionHandler,transactionMessage);
    }
    return transactionMessage;
}

TransactionMessageHelperPtr TransactionMessageHelper::decryptMessage(TransactionEncryptionHandler& 
        transactionEncryptionHandler) {
    TransactionMessageHelperPtr transactionMessageHelperPtr(new TransactionMessageHelper());
    transactionMessageHelperPtr->setTransactionWrapper(keto::asn1::clone<TransactionWrapper_t>(
            &this->transactionMessage->transaction,&asn_DEF_TransactionWrapper));
    
    for (int index = 0; index < this->transactionMessage->nestedTransactions.list.count; index++) {
        TransactionMessage__nestedTransactions__Member* nestedTransaction =
            this->transactionMessage->nestedTransactions.list.array[index];
        decryptMessage(transactionEncryptionHandler,transactionMessageHelperPtr,nestedTransaction);
    }
    return transactionMessageHelperPtr;
}

bool TransactionMessageHelper::isEncrypted() {
    return this->encrypt;
}


void TransactionMessageHelper::getMessage(TransactionEncryptionHandler& 
            transactionEncryptionHandler,TransactionMessage_t* transactionMessage) {
    TransactionMessage_t* _transactionMessage = 
            keto::asn1::clone<TransactionMessage_t>(this->transactionMessage,&asn_DEF_TransactionMessage);
    for (TransactionMessageHelperPtr transactionMessageHelperPtr : nestedTransactions) {
        transactionMessageHelperPtr->getMessage(transactionEncryptionHandler,_transactionMessage);
    }
    
    TransactionMessage__nestedTransactions__Member* nestedTransaction =
            (TransactionMessage__nestedTransactions__Member*)calloc(1, sizeof *nestedTransaction);
    
    if (this->isEncrypted()) {
        nestedTransaction->present = TransactionMessage__nestedTransactions__Member_PR_encryptedSideTransaction;
        nestedTransaction->choice.encryptedSideTransaction = *transactionEncryptionHandler.encrypt(*_transactionMessage);
        ASN_STRUCT_FREE(asn_DEF_TransactionMessage, _transactionMessage);
    } else {
        nestedTransaction->present = TransactionMessage__nestedTransactions__Member_PR_sideTransaction;
        nestedTransaction->choice.sideTransaction = _transactionMessage;
    }
    
    ASN_SEQUENCE_ADD(&transactionMessage->nestedTransactions,nestedTransaction);
}


void TransactionMessageHelper::decryptMessage(TransactionEncryptionHandler&
        transactionEncryptionHandler, TransactionMessageHelperPtr transactionMessageHelperPtr, 
        TransactionMessage__nestedTransactions__Member* nestedTransaction) {
    TransactionMessage_t* transactionMessage = NULL;
    bool encrypted = false;
    if (nestedTransaction->present == TransactionMessage__nestedTransactions__Member_PR_NOTHING) {
        return;
    } else if (nestedTransaction->present == TransactionMessage__nestedTransactions__Member_PR_sideTransaction) {
        transactionMessage = 
                keto::asn1::clone<TransactionMessage_t>(nestedTransaction->choice.sideTransaction,&asn_DEF_TransactionMessage);
    } else if (nestedTransaction->present == TransactionMessage__nestedTransactions__Member_PR_encryptedSideTransaction) {
        transactionMessage = 
                transactionEncryptionHandler.decrypt(nestedTransaction->choice.encryptedSideTransaction);
        encrypted = true;
    }
    
    TransactionMessageHelperPtr transactionMessageHelperPtr_(new TransactionMessageHelper(
            transactionMessage));
    transactionMessageHelperPtr_->setEncrypted(encrypted);
    
    for (int index = 0; index < transactionMessage->nestedTransactions.list.count; index++) {
        TransactionMessage__nestedTransactions__Member* nestedTransaction =
            transactionMessage->nestedTransactions.list.array[index];    
        decryptMessage(transactionEncryptionHandler, transactionMessageHelperPtr_, 
                nestedTransaction);
    }
    
    transactionMessageHelperPtr->addNestedTransaction(transactionMessageHelperPtr_);
}


}
}
