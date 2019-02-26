/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionWrapperHelper.cpp
 * Author: ubuntu
 * 
 * Created on March 17, 2018, 4:21 AM
 */

#include <TransactionWrapper.h>
#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/transaction_common/Exception.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/DeserializationHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"

namespace keto {
namespace transaction_common {

std::string TransactionWrapperHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
TransactionWrapperHelper::TransactionWrapperHelper() {
    this->own = true;
    this->transactionWrapper = (TransactionWrapper_t*)calloc(1, sizeof *transactionWrapper);
    this->transactionWrapper->version = keto::common::MetaInfo::PROTOCOL_VERSION;
}


TransactionWrapperHelper::TransactionWrapperHelper(SignedTransaction_t* signedTransaction) {
    this->own = true;
    this->transactionWrapper = (TransactionWrapper_t*)calloc(1, sizeof *transactionWrapper);
    this->transactionWrapper->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionWrapper->signedTransaction = *signedTransaction;
    this->transactionWrapper->transactionHash = keto::asn1::HashHelper(
            signedTransaction->transactionHash);
    this->transactionWrapper->signature = keto::asn1::SignatureHelper(
            signedTransaction->signature);
    this->transactionWrapper->parent = keto::asn1::HashHelper(
            signedTransaction->transaction.parent);
    this->transactionWrapper->sourceAccount = keto::asn1::HashHelper(
            signedTransaction->transaction.sourceAccount);
    this->transactionWrapper->targetAccount = keto::asn1::HashHelper(
            signedTransaction->transaction.targetAccount);
}


TransactionWrapperHelper::TransactionWrapperHelper(SignedTransaction_t* signedTransaction,
        const keto::asn1::HashHelper& sourceAccount, 
        const keto::asn1::HashHelper& targetAccount) {
    this->own = true;
    this->transactionWrapper = (TransactionWrapper_t*)calloc(1, sizeof *transactionWrapper);
    this->transactionWrapper->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->transactionWrapper->signedTransaction = *signedTransaction;
    this->transactionWrapper->transactionHash = keto::asn1::HashHelper(
            signedTransaction->transactionHash);
    this->transactionWrapper->signature = keto::asn1::SignatureHelper(
            signedTransaction->signature);
    this->transactionWrapper->parent = keto::asn1::HashHelper(
            signedTransaction->transaction.parent);
    this->transactionWrapper->sourceAccount = sourceAccount;
    this->transactionWrapper->targetAccount = targetAccount;
    
}

TransactionWrapperHelper::TransactionWrapperHelper(const TransactionWrapper_t& transactionWrapper) {
    this->transactionWrapper = keto::asn1::clone<TransactionWrapper_t>(&transactionWrapper,&asn_DEF_TransactionWrapper);
    this->own = true;
}

TransactionWrapperHelper::TransactionWrapperHelper(TransactionWrapper_t* transactionWrapper) :
    transactionWrapper(transactionWrapper) {
    this->own = true;
    
}

TransactionWrapperHelper::TransactionWrapperHelper(TransactionWrapper_t* transactionWrapper, bool own) :
    transactionWrapper(transactionWrapper) {
    this->own = own;
}


TransactionWrapperHelper::TransactionWrapperHelper(const std::string& transactionWrapper) {
    this->own = true;
    if (!transactionWrapper.size()) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::InvalidTransactionDataException());
    }
    this->transactionWrapper = 
            keto::asn1::DeserializationHelper<TransactionWrapper_t>((const uint8_t*)transactionWrapper.data(), 
            transactionWrapper.size(),&asn_DEF_TransactionWrapper).takePtr();
}

TransactionWrapperHelper::~TransactionWrapperHelper() {
    if (this->own && transactionWrapper) {
        ASN_STRUCT_FREE(asn_DEF_TransactionWrapper, transactionWrapper);
        transactionWrapper = 0;
    }
}


TransactionWrapperHelper& TransactionWrapperHelper::setSignedTransaction(
    SignedTransaction_t* signedTransaction) {
    this->transactionWrapper->signedTransaction = *signedTransaction;
    this->transactionWrapper->transactionHash = keto::asn1::HashHelper(
            signedTransaction->transactionHash);
    this->transactionWrapper->signature = keto::asn1::SignatureHelper(
            signedTransaction->signature);
    this->transactionWrapper->parent = keto::asn1::HashHelper(
            signedTransaction->transaction.parent);

    return (*this);
}

TransactionWrapperHelper& TransactionWrapperHelper::setSourceAccount(
    const keto::asn1::HashHelper& sourceAccount) {
    this->transactionWrapper->sourceAccount = sourceAccount;
    return (*this);
}

TransactionWrapperHelper& TransactionWrapperHelper::setTargetAccount(
    const keto::asn1::HashHelper& targetAccount) {
    this->transactionWrapper->targetAccount = targetAccount;
    return (*this);
}

TransactionWrapperHelper& TransactionWrapperHelper::setFeeAccount(
    const keto::asn1::HashHelper& feeAccount) {
    this->transactionWrapper->feeAccount = feeAccount;
    return (*this);
}

TransactionWrapperHelper& TransactionWrapperHelper::setStatus(const Status& status) {
    this->transactionWrapper->currentStatus = status;
    return (*this);
}

TransactionWrapperHelper& TransactionWrapperHelper::addTransactionTrace(
    TransactionTrace_t* transactionTrace) {
    if (0!= ASN_SEQUENCE_ADD(&this->transactionWrapper->transactionTrace,transactionTrace)) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::TransactionTraceSequenceAddFailedException());
    }
    return (*this);
}

TransactionWrapperHelper& TransactionWrapperHelper::addChangeSet(
    SignedChangeSet_t* signedChangeSet) {
    
    if (0!= ASN_SEQUENCE_ADD(&this->transactionWrapper->changeSet,signedChangeSet)) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::SignedChangeSetSequenceAddFailedException());
    }
    return (*this);
}

std::vector<SignedChangeSetHelperPtr> TransactionWrapperHelper::getChangeSets() {
    std::vector<SignedChangeSetHelperPtr> result;
    for(int index =0; index < this->transactionWrapper->changeSet.list.count; index++) {
        result.push_back(SignedChangeSetHelperPtr(new SignedChangeSetHelper(this->transactionWrapper->changeSet.list.array[index])));
    }

    return result;
}

TransactionWrapperHelper& TransactionWrapperHelper::operator =(const std::string& transactionWrapper) {
    if (!transactionWrapper.size()) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::InvalidTransactionDataException());
    }
    this->transactionWrapper =
            keto::asn1::DeserializationHelper<TransactionWrapper_t>((const uint8_t*)transactionWrapper.data(),
                    transactionWrapper.size(),&asn_DEF_TransactionWrapper).takePtr();
    return (*this);
}

TransactionWrapperHelper::operator TransactionWrapper_t&() {
    return (*this->transactionWrapper);
}

TransactionWrapperHelper::operator TransactionWrapper_t*() {
    return keto::asn1::clone<TransactionWrapper_t>(transactionWrapper,&asn_DEF_TransactionWrapper);
    //TransactionWrapper_t* result = this->transactionWrapper;
    //this->transactionWrapper = 0;
    //return result;
}

TransactionWrapperHelper::operator std::vector<uint8_t>() {
    return keto::asn1::SerializationHelper<TransactionWrapper>(this->transactionWrapper,
        &asn_DEF_TransactionWrapper).operator std::vector<uint8_t>&();
}


TransactionWrapperHelper::operator std::string() {
    std::vector<uint8_t> bytes = keto::asn1::SerializationHelper<TransactionWrapper>(this->transactionWrapper,
                                                               &asn_DEF_TransactionWrapper);
    return keto::server_common::VectorUtils().copyVectorToString(bytes);
}
    

TransactionWrapperHelper::operator ANY_t*() {
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_TransactionWrapper, this->transactionWrapper);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::ANYSerializationFailedException());
    }
    return anyPtr;
}

keto::asn1::HashHelper TransactionWrapperHelper::getSourceAccount() {
    return this->transactionWrapper->sourceAccount;
}


keto::asn1::HashHelper TransactionWrapperHelper::getTargetAccount() {
    return this->transactionWrapper->targetAccount;
}

keto::asn1::HashHelper TransactionWrapperHelper::getFeeAccount() {
    return this->transactionWrapper->feeAccount;
}

keto::asn1::HashHelper TransactionWrapperHelper::getHash() {
    return this->transactionWrapper->transactionHash;
}


keto::asn1::HashHelper TransactionWrapperHelper::getParentHash() {
    return this->transactionWrapper->parent;
}

keto::asn1::SignatureHelper TransactionWrapperHelper::getSignature() {
    return this->transactionWrapper->signature;
}

Status TransactionWrapperHelper::getStatus() {
    return (Status)this->transactionWrapper->currentStatus;
}


keto::asn1::HashHelper TransactionWrapperHelper::getCurrentAccount() {
    if ((getStatus() == Status_init) ||
        (getStatus() == Status_debit ) ||
        (getStatus() == Status_processing)){
        return getSourceAccount();
    } else if (getStatus() == Status_credit || getStatus() == Status_complete) {
        return getTargetAccount();
    }
    std::stringstream ss;
    ss << "Unrecognised status [" << getStatus() << "]";
    BOOST_THROW_EXCEPTION(keto::transaction_common::UnrecognisedTransactionStatusException(
                        ss.str()));
}


Status TransactionWrapperHelper::incrementStatus() {
    if ((this->transactionWrapper->currentStatus == Status_init) ||
        (this->transactionWrapper->currentStatus == Status_debit)){
        this->transactionWrapper->currentStatus = Status_processing;
    } else if (this->transactionWrapper->currentStatus == Status_processing) {
        this->transactionWrapper->currentStatus = Status_credit;
    } else if (this->transactionWrapper->currentStatus == Status_credit) {
        this->transactionWrapper->currentStatus = Status_complete;
    }
    return (Status)this->transactionWrapper->currentStatus;
}

SignedTransactionHelperPtr TransactionWrapperHelper::getSignedTransaction() {
    return SignedTransactionHelperPtr(
            new SignedTransactionHelper(&this->transactionWrapper->signedTransaction));
}

}
}
