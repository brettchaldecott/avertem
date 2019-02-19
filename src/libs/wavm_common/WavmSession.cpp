/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmSession.cpp
 * Author: ubuntu
 * 
 * Created on May 3, 2018, 2:05 PM
 */

#include <cstdlib>
#include <sstream>

#include "RDFChange.h"

#include "keto/environment/Units.hpp"
#include "keto/asn1/StatusUtils.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/wavm_common/WavmSession.hpp"
#include "keto/wavm_common/RDFURLUtils.hpp"
#include "keto/wavm_common/RDFConstants.hpp"
#include "include/keto/wavm_common/RDFConstants.hpp"
#include "include/keto/wavm_common/RDFURLUtils.hpp"
#include "keto/wavm_common/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/ChangeSetBuilder.hpp"
#include "keto/transaction_common/SignedChangeSetBuilder.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

namespace keto {
namespace wavm_common {

std::string WavmSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
WavmSession::WavmSession(const keto::proto::SandboxCommandMessage& sandboxCommandMessage,
        const keto::crypto::KeyLoaderPtr& keyLoaderPtr) : 
    sandboxCommandMessage(sandboxCommandMessage) , modelHelper(RDFChange_persist),
        keyLoaderPtr(keyLoaderPtr) {

    this->startTime = std::chrono::high_resolution_clock::now();

    transactionProtoHelper.setTransaction(sandboxCommandMessage.transaction());
    transactionMessageHelperPtr = transactionProtoHelper.getTransactionMessageHelper();
    rdfSessionPtr = std::make_shared<RDFMemorySession>();
    if (sandboxCommandMessage.model().size()) {
        keto::asn1::AnyHelper anyHelper(sandboxCommandMessage.model());
        RDFModel_t* rdfModel;
        TransactionMessage_t* transactionMessage;
        if ((rdfModel = anyHelper.extract<RDFModel_t>(&asn_DEF_RDFModel)) != NULL) {
            keto::asn1::RDFModelHelper rdfModelHelper(rdfModel);
            for (keto::asn1::RDFSubjectHelperPtr subject : rdfModelHelper.getSubjects()) {
                rdfSessionPtr->persist(subject);
            }
        } else if ((transactionMessage = anyHelper.extract<TransactionMessage_t>(&asn_DEF_TransactionMessage)) != NULL) {
            keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr(
                    new keto::transaction_common::TransactionMessageHelper(transactionMessage));
            std::cout << "Set the transaction model" << std::endl;
            addTransaction(transactionMessageHelperPtr,true);
        }
    }
    std::cout << "Setup the existing transaction" << std::endl;
    addTransaction(transactionMessageHelperPtr,false);
}

WavmSession::~WavmSession() {
    
}

// the contract facade methods
std::string WavmSession::getAccount() {
    return getCurrentAccountHash().getHash(keto::common::StringEncoding::HEX);
}

std::string WavmSession::getTransaction() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getSignedTransaction()->getHash().getHash(keto::common::StringEncoding::HEX);
}

Status WavmSession::getStatus() {
    if (!transactionMessageHelperPtr->getTransactionWrapper()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidTransactionReferenceException());
    }
    return transactionMessageHelperPtr->getTransactionWrapper()->getStatus();
}

keto::asn1::NumberHelper WavmSession::getTransactionValue() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getSignedTransaction()->getTransaction()->getValue();
}

keto::asn1::NumberHelper WavmSession::getTransactionFee() {
    keto::asn1::NumberHelper numberHelper((sandboxCommandMessage.elapsed_time() / keto::environment::Units::TIME::MILLISECONDS) * sandboxCommandMessage.fee_ratio());
    return numberHelper;
}

// request methods
long WavmSession::getRequestModelTransactionValue(
    const std::string& accountModel,
    const std::string& transactionValueModel) {
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string transactionId = getTransaction();
    return rdfSessionPtr->getLongValue(transactionUrl.buildSubjectUrl(transactionId),
            transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE));
}

std::string WavmSession::getRequestStringValue(const std::string& subject, 
        const std::string& predicate) {
    return rdfSessionPtr->getStringValue(subject,predicate);
}

long WavmSession::getRequestLongValue(const std::string& subject, 
        const std::string& predicate) {
    return rdfSessionPtr->getLongValue(subject,predicate);
}

float WavmSession::getRequestFloatValue(const std::string& subject,
        const std::string& predicate) {
    return rdfSessionPtr->getFloatValue(subject,predicate);
}

bool WavmSession::getRequestBooleanValue(const std::string& subject,
        const std::string& predicate) {
    return rdfSessionPtr->getBooleanValue(subject,predicate);
}



void WavmSession::createDebitEntry(const std::string& accountModel, const std::string& transactionValueModel,
        const keto::asn1::NumberHelper& value) {
    keto::wavm_common::RDFURLUtils accountUrl(accountModel);
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string id = this->getTransaction();
    std::string hash = this->getAccount();
    
    std::stringstream ss;
    ss << "debit_" << hash << "_" << id;
    std::string subjectUrl = transactionUrl.buildSubjectUrl(ss.str());
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ID),id);
    this->addDateTimeModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::DATE_TIME),time(0));
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::TYPE),
            keto::server_common::Constants::Constants::ACCOUNT_ACTIONS::DEBIT);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ACCOUNT_HASH),
            hash);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE),
            value.operator long());
    
}

void WavmSession::createCreditEntry(const std::string& accountModel, const std::string& transactionValueModel,
        const keto::asn1::NumberHelper& value) {
    keto::wavm_common::RDFURLUtils accountUrl(accountModel);
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string id = this->getTransaction();
    std::string hash = this->getAccount();
    std::stringstream ss;
    ss << "credit_" << hash << "_" << id;
    std::string subjectUrl = transactionUrl.buildSubjectUrl(ss.str());
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ID),id);
    this->addDateTimeModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::DATE_TIME),time(0));
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::TYPE),
            keto::server_common::Constants::Constants::ACCOUNT_ACTIONS::CREDIT);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ACCOUNT_HASH),
            hash);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE),
            value.operator long());
    
}

void WavmSession::setResponseStringValue(const std::string& subject, const std::string& predicate,
        const std::string& value) {
    this->addModelEntry(subject,predicate,value);
}

void WavmSession::setResponseLongValue(const std::string& subject, const std::string& predicate,
        const long value) {
    this->addModelEntry(subject,predicate,value);
}

void WavmSession::setResponseFloatValue(const std::string& subject, const std::string& predicate,
        const float value) {
    this->addModelEntry(subject,predicate,value);
}

void WavmSession::setResponseBooleanValue(const std::string& subject, const std::string& predicate,
        const bool value) {
    this->addBooleanModelEntry(subject,predicate,value);
}


keto::proto::SandboxCommandMessage WavmSession::getSandboxCommandMessage() {
    // create a change and add it to the transaction
    keto::asn1::AnyHelper anyModel(this->modelHelper);
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr =
            transactionMessageHelperPtr->getTransactionWrapper();
    keto::transaction_common::ChangeSetBuilderPtr changeSetBuilder(
        new keto::transaction_common::ChangeSetBuilder(
            transactionWrapperHelperPtr->getSignedTransaction()->getHash(),
            this->getCurrentAccountHash()));
    changeSetBuilder->addChange(anyModel).setStatus(transactionWrapperHelperPtr->getStatus());
    keto::transaction_common::SignedChangeSetBuilderPtr signedChangeSetBuilder(new
        keto::transaction_common::SignedChangeSetBuilder(*changeSetBuilder,*keyLoaderPtr));
    signedChangeSetBuilder->sign();
    transactionWrapperHelperPtr->addChangeSet(*signedChangeSetBuilder);
    
    // set the the transaction
    keto::transaction_common::TransactionProtoHelper transactionProtoHelper(
                transactionMessageHelperPtr);
    sandboxCommandMessage.set_transaction(transactionProtoHelper.operator std::string());

    // calculate the elapsed time
    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    sandboxCommandMessage.set_elapsed_time(sandboxCommandMessage.elapsed_time() + (endTime.count()));
    
    return this->sandboxCommandMessage;
}


keto::asn1::RDFSubjectHelperPtr WavmSession::getSubject(const std::string& subjectUrl) {
    if (!this->modelHelper.contains(subjectUrl)) {
        keto::asn1::RDFSubjectHelper subject(subjectUrl);
        this->modelHelper.addSubject(subject);
    }
    return this->modelHelper[subjectUrl];
}

keto::asn1::RDFPredicateHelperPtr WavmSession::getPredicate(
    keto::asn1::RDFSubjectHelperPtr subject, const std::string& predicateUrl) {
    if (!subject->containsPredicate(predicateUrl)) {
        keto::asn1::RDFPredicateHelper predicate(predicateUrl);
        subject->addPredicate(predicate);
    }
    return subject->operator [](predicateUrl);
}

void WavmSession::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const std::string& value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    
    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::STRING).
        setType(RDFConstants::NODE_TYPES::LITERAL).
        setValue(value);
    
    predicate->addObject(objectHelper);
    
    rdfSessionPtr->setStringValue(
        subjectUrl,predicateUrl,value);
    
    
}

void WavmSession::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const long value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    
    std::stringstream ss;
    ss << value;
    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::LONG).
        setType(RDFConstants::NODE_TYPES::LITERAL).
        setValue(ss.str());
    
    predicate->addObject(objectHelper);
    
    rdfSessionPtr->setLongValue(
        subjectUrl,predicateUrl,value);
}

void WavmSession::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const float value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    
    std::stringstream ss;
    ss << value;
    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::FLOAT).
        setType(RDFConstants::NODE_TYPES::LITERAL).
        setValue(ss.str());
    
    predicate->addObject(objectHelper);
    
    rdfSessionPtr->setFloatValue(
        subjectUrl,predicateUrl,value);
    
}

void WavmSession::addBooleanModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const bool value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    
    std::stringstream ss;
    if (value) {
        ss << "true";
    } else {
        ss << "false";
    }
    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::BOOLEAN).
        setType(RDFConstants::NODE_TYPES::LITERAL).
        setValue(ss.str());
    
    predicate->addObject(objectHelper);
    
    rdfSessionPtr->setBooleanValue(
        subjectUrl,predicateUrl,value);
}

void WavmSession::addDateTimeModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const time_t value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    
    struct tm  tstruct;
    char       buf[80];
    struct tm result;
    localtime_r(&value,&result);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%X", &tstruct);
    
    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::DATE_TIME).
        setType(RDFConstants::NODE_TYPES::LITERAL).
        setValue(buf);
    
    predicate->addObject(objectHelper);
    
    rdfSessionPtr->setDateTimeValue(
        subjectUrl,predicateUrl,value);
}


keto::asn1::HashHelper WavmSession::getCurrentAccountHash() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getCurrentAccount();
}


void WavmSession::addTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr,
                                 bool defineRdfChangeSet) {
    RDFURLUtils rdfurlUtils(RDFConstants::CHANGE_SET_SUBJECT);
    for (keto::transaction_common::SignedChangeSetHelperPtr signedChangeSetHelperPtr :
            transactionMessageHelperPtr->getTransactionWrapper()->getChangeSets()) {
        keto::asn1::ChangeSetHelperPtr changeSetHelperPtr = signedChangeSetHelperPtr->getChangeSetHelper();
        std::cout << "Get all the hashes for the signature" << std::endl;
        std::string hash = signedChangeSetHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX);

        if (defineRdfChangeSet) {
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildSubjectUrl(RDFConstants::CHANGE_SET_PREDICATES::ID), hash);
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildSubjectUrl(RDFConstants::CHANGE_SET_PREDICATES::CHANGE_SET_HASH), hash);
            rdfSessionPtr->setDateTimeValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildSubjectUrl(RDFConstants::CHANGE_SET_PREDICATES::DATE_TIME), time(0));
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildSubjectUrl(RDFConstants::CHANGE_SET_PREDICATES::TYPE),
                    keto::asn1::StatusUtils::statusToString(
                            changeSetHelperPtr->getStatus()));

        }

        for (keto::asn1::ChangeSetDataHelperPtr changeSetDataHelperPtr:
                changeSetHelperPtr->getChanges()) {
            if (!changeSetDataHelperPtr->isASN1()) {
                continue;
            }

            keto::asn1::RDFModelHelper rdfModelHelper(
                    changeSetDataHelperPtr->getAny());
            for (keto::asn1::RDFSubjectHelperPtr subject : rdfModelHelper.getSubjects()) {
                rdfSessionPtr->persist(subject);
                if (defineRdfChangeSet) {
                    rdfSessionPtr->setStringValue(
                            rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildSubjectUrl(RDFConstants::CHANGE_SET_PREDICATES::URI),
                            subject->getSubject());
                }
            }
        }
    }
}

}
}
