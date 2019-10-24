/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmSessionTransaction.cpp
 * Author: ubuntu
 * 
 * Created on May 3, 2018, 2:05 PM
 */

#include <cstdlib>
#include <sstream>
#include <math.h>
#include <time.h>

#include <botan/hex.h>

#include <keto/wavm_common/Constants.hpp>
#include <keto/crypto/HashGenerator.hpp>

#include "RDFChange.h"

#include "keto/environment/Units.hpp"
#include "keto/asn1/StatusUtils.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/RDFUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"

#include "keto/wavm_common/WavmSessionTransaction.hpp"
#include "keto/wavm_common/RDFURLUtils.hpp"
#include "keto/wavm_common/RDFConstants.hpp"
#include "keto/wavm_common/Exception.hpp"
#include "keto/account_query/AccountSparqlQueryHelper.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/transaction_common/Constants.hpp"
#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/ChangeSetBuilder.hpp"
#include "keto/transaction_common/SignedChangeSetBuilder.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

namespace keto {
namespace wavm_common {

std::string WavmSessionTransaction::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
WavmSessionTransaction::WavmSessionTransaction(const keto::proto::SandboxCommandMessage& sandboxCommandMessage,
        const keto::crypto::KeyLoaderPtr& keyLoaderPtr) : 
    sandboxCommandMessage(sandboxCommandMessage) , modelHelper(RDFChange_persist),
        keyLoaderPtr(keyLoaderPtr) {

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
            addTransaction(transactionMessageHelperPtr,true);
        }
    }
    addTransaction(transactionMessageHelperPtr,false);
    this->contractHash = keto::asn1::HashHelper(this->sandboxCommandMessage.contract_hash());

}

WavmSessionTransaction::~WavmSessionTransaction() {
    
}

std::string WavmSessionTransaction::getSessionType() {
    return Constants::SESSION_TYPES::TRANSACTION;
}

// the contract facade methods
std::string WavmSessionTransaction::getFeeAccount() {
    return keto::server_common::ServerInfo::getInstance()->getFeeAccountHashHex();
}

// the contract facade methods
std::string WavmSessionTransaction::getAccount() {
    return getCurrentAccountHash().getHash(keto::common::StringEncoding::HEX);
}

std::string WavmSessionTransaction::getTransaction() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getSignedTransaction()->getHash().getHash(keto::common::StringEncoding::HEX);
}

Status WavmSessionTransaction::getStatus() {
    if (!transactionMessageHelperPtr->getTransactionWrapper()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidTransactionReferenceException());
    }
    return transactionMessageHelperPtr->getTransactionWrapper()->getStatus();
}

keto::asn1::NumberHelper WavmSessionTransaction::getTransactionValue() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getSignedTransaction()->getTransaction()->getValue();
}

keto::asn1::NumberHelper WavmSessionTransaction::getTotalTransactionFee(long minimimFee) {
    keto::asn1::NumberHelper numberHelper(
            ((minimimFee * 2 ) + transactionMessageHelperPtr->getElapsedTime()) * sandboxCommandMessage.fee_ratio());
    return numberHelper;
}

keto::asn1::NumberHelper WavmSessionTransaction::getTransactionFee(long minimimFee) {
    keto::asn1::NumberHelper numberHelper(
            round(
                    ((minimimFee + (sandboxCommandMessage.elapsed_time() / keto::environment::Units::TIME::MILLISECONDS)) *
                    sandboxCommandMessage.fee_ratio())));
    return numberHelper;
}

// request methods
long WavmSessionTransaction::getRequestModelTransactionValue(
    const std::string& accountModel,
    const std::string& transactionValueModel) {
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string transactionId = getTransaction();
    return rdfSessionPtr->getLongValue(transactionUrl.buildSubjectUrl(transactionId),
            transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE));
}

std::string WavmSessionTransaction::getRequestStringValue(const std::string& subject,
        const std::string& predicate) {
    return rdfSessionPtr->getStringValue(subject,predicate);
}

long WavmSessionTransaction::getRequestLongValue(const std::string& subject,
        const std::string& predicate) {
    return rdfSessionPtr->getLongValue(subject,predicate);
}

float WavmSessionTransaction::getRequestFloatValue(const std::string& subject,
        const std::string& predicate) {
    return rdfSessionPtr->getFloatValue(subject,predicate);
}

bool WavmSessionTransaction::getRequestBooleanValue(const std::string& subject,
        const std::string& predicate) {
    return rdfSessionPtr->getBooleanValue(subject,predicate);
}


void WavmSessionTransaction::createDebitEntry(const std::string& accountId, const std::string& name, const std::string& description, const std::string& accountModel, const std::string& transactionValueModel,
        const keto::asn1::NumberHelper& value) {
    keto::wavm_common::RDFURLUtils accountUrl(accountModel);
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string id = this->getTransaction();
    std::string hash = accountId;
    time_t transactionTime = time(0);

    std::stringstream ss;
    ss << "debit_" << accountId << name << description << accountModel << transactionValueModel << transactionTime << (long)value <<  "" << id;
    keto::asn1::HashHelper transactionHash(keto::crypto::HashGenerator().generateHash(ss.str()));

    std::string subjectUrl = transactionUrl.buildSubjectUrl(transactionHash.getHash(keto::common::StringEncoding::HEX));

    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ID),id);
    this->addDateTimeModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::DATE_TIME),transactionTime);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::TYPE),
            keto::server_common::Constants::Constants::ACCOUNT_ACTIONS::DEBIT);
    this->addModelEntry(
            subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::NAME),
            name);
    this->addModelEntry(
            subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::DESCRIPTION),
            description);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ACCOUNT_HASH),
            hash);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE),
            value.operator long());
    
}

void WavmSessionTransaction::createCreditEntry(const std::string& accountId, const std::string& name, const std::string& description, const std::string& accountModel, const std::string& transactionValueModel,
        const keto::asn1::NumberHelper& value) {
    keto::wavm_common::RDFURLUtils accountUrl(accountModel);
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string id = this->getTransaction();
    std::string hash = accountId;
    time_t transactionTime = time(0);

    std::stringstream ss;
    ss << "credit_" << accountId << name << description << accountModel << transactionValueModel << transactionTime << (long)value <<  "" << id;
    keto::asn1::HashHelper transactionHash(keto::crypto::HashGenerator().generateHash(ss.str()));

    std::string subjectUrl = transactionUrl.buildSubjectUrl(transactionHash.getHash(keto::common::StringEncoding::HEX));
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ID),id);
    this->addDateTimeModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::DATE_TIME),transactionTime);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::TYPE),
            keto::server_common::Constants::Constants::ACCOUNT_ACTIONS::CREDIT);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ACCOUNT_HASH),
            hash);
    this->addModelEntry(
            subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::NAME),
            name);
    this->addModelEntry(
            subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::DESCRIPTION),
            description);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE),
            value.operator long());
    
}


void WavmSessionTransaction::setResponseStringValue(const std::string& subject, const std::string& predicate,
        const std::string& value) {
    this->addModelEntry(subject,predicate,value);
}

void WavmSessionTransaction::setResponseLongValue(const std::string& subject, const std::string& predicate,
        const long value) {
    this->addModelEntry(subject,predicate,value);
}

void WavmSessionTransaction::setResponseFloatValue(const std::string& subject, const std::string& predicate,
        const float value) {
    this->addModelEntry(subject,predicate,value);
}

void WavmSessionTransaction::setResponseBooleanValue(const std::string& subject, const std::string& predicate,
        const bool value) {
    this->addBooleanModelEntry(subject,predicate,value);
}

long WavmSessionTransaction::executeQuery(const std::string& type, const std::string& query) {
    if (type == Constants::SESSION_SPARQL_QUERY) {
        return addResultVectorMap(this->rdfSessionPtr->executeQuery(query));
    } else {
        return addResultVectorMap(keto::account_query::AccountSparqlQueryHelper(keto::server_common::Events::DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
                getCurrentAccountHash(),query).execute());
    }
}


keto::proto::SandboxCommandMessage WavmSessionTransaction::getSandboxCommandMessage() {
    // create a change and add it to the transaction
    keto::asn1::AnyHelper anyModel = this->modelHelper;
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr =
            transactionMessageHelperPtr->getTransactionWrapper();
    keto::transaction_common::ChangeSetBuilderPtr changeSetBuilder(
        new keto::transaction_common::ChangeSetBuilder(
            transactionWrapperHelperPtr->getSignedTransaction()->getHash(),
            this->getCurrentAccountHash()));
    changeSetBuilder->addChange(anyModel).setStatus(transactionWrapperHelperPtr->getStatus());
    keto::transaction_common::SignedChangeSetBuilderPtr signedChangeSetBuilder(new
        keto::transaction_common::SignedChangeSetBuilder(*changeSetBuilder,keyLoaderPtr));
    signedChangeSetBuilder->sign();
    transactionWrapperHelperPtr->addChangeSet(*signedChangeSetBuilder);
    
    // set the the transaction
    keto::transaction_common::TransactionProtoHelper transactionProtoHelper(
                transactionMessageHelperPtr);
    sandboxCommandMessage.set_transaction(transactionProtoHelper.operator std::string());

    // calculate the elapsed time
    std::chrono::milliseconds elapsedTime = getExecutionTime();

    sandboxCommandMessage.set_elapsed_time(sandboxCommandMessage.elapsed_time() + (elapsedTime.count()));
    
    return this->sandboxCommandMessage;
}

// create a transaction
WavmSessionTransactionBuilderPtr WavmSessionTransaction::createChildTransaction() {
    int id = this->childrenTransactions.size();
    WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr(new WavmSessionTransactionBuilder(id,
            transactionMessageHelperPtr->getTransactionWrapper()->getSignedTransaction()->getHash(),this->keyLoaderPtr));
    this->childrenTransactions.push_back(wavmSessionTransactionBuilderPtr);
    return wavmSessionTransactionBuilderPtr;
}

WavmSessionTransactionBuilderPtr WavmSessionTransaction::getChildTransaction(int id) {
    if (id >= this->childrenTransactions.size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidTransactionIdForThisSession());
    }
    return this->childrenTransactions[id];
}


keto::asn1::RDFSubjectHelperPtr WavmSessionTransaction::getSubject(const std::string& subjectUrl) {
    if (!this->modelHelper.contains(subjectUrl)) {
        validateSubject(subjectUrl);
        keto::asn1::RDFSubjectHelper subject(subjectUrl);
        this->modelHelper.addSubject(subject);
    }
    return this->modelHelper[subjectUrl];
}

void WavmSessionTransaction::validateSubject(const std::string& subjectUrl) {
    for (const char* contractName : Constants::SYSTEM_CONTRACTS) {
        if (this->sandboxCommandMessage.contract_name() == contractName){
            return;
        }
    }
    if (subjectUrl.find(this->sandboxCommandMessage.contract_name()) != std::string::npos) {
        return;
    }
    if (subjectUrl.find(this->contractHash.getHash(keto::common::StringEncoding::HEX)) != std::string::npos) {
        return;
    }
    BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidSubjectForContract());
}

keto::asn1::RDFPredicateHelperPtr WavmSessionTransaction::getPredicate(
    keto::asn1::RDFSubjectHelperPtr subject, const std::string& predicateUrl) {
    if (!subject->containsPredicate(predicateUrl)) {
        keto::asn1::RDFPredicateHelper predicate(predicateUrl);
        subject->addPredicate(predicate);
    }
    return (*subject)[predicateUrl];
}

void WavmSessionTransaction::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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

void WavmSessionTransaction::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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

void WavmSessionTransaction::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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

void WavmSessionTransaction::addBooleanModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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

void WavmSessionTransaction::addDateTimeModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const time_t value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);

    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::DATE_TIME).
        setType(RDFConstants::NODE_TYPES::LITERAL).
        setValue(keto::server_common::RDFUtils::convertTimeToRDFDateTime(value));
    predicate->addObject(objectHelper);
    
    rdfSessionPtr->setDateTimeValue(
        subjectUrl,predicateUrl,value);
}


keto::asn1::HashHelper WavmSessionTransaction::getCurrentAccountHash() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getCurrentAccount();
}


void WavmSessionTransaction::addTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr,
                                 bool defineRdfChangeSet) {
    RDFURLUtils rdfurlUtils(RDFConstants::CHANGE_SET_SUBJECT);
    for (keto::transaction_common::SignedChangeSetHelperPtr signedChangeSetHelperPtr :
            transactionMessageHelperPtr->getTransactionWrapper()->getChangeSets()) {
        keto::asn1::ChangeSetHelperPtr changeSetHelperPtr = signedChangeSetHelperPtr->getChangeSetHelper();
        std::string hash = signedChangeSetHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX);

        if (defineRdfChangeSet) {
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::ID), hash);
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::CHANGE_SET_HASH), hash);
            rdfSessionPtr->setDateTimeValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::DATE_TIME), time(0));
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::TYPE),
                    keto::asn1::StatusUtils::statusToString(
                            changeSetHelperPtr->getStatus()));
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::SIGNATURE),
                            signedChangeSetHelperPtr->getSignature().getSignature(keto::common::HEX));
            rdfSessionPtr->setStringValue(
                    rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::TRANSACTION_HASH),
                    transactionMessageHelperPtr->getTransactionWrapper()->getHash().getHash(keto::common::HEX));
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
                            rdfurlUtils.buildSubjectUrl(hash), rdfurlUtils.buildPredicateUrl(RDFConstants::CHANGE_SET_PREDICATES::URI),
                            subject->getSubject());
                }
            }
        }
    }
}


std::vector<std::string> WavmSessionTransaction::getKeys(ResultVectorMap& resultVectorMap) {
    std::vector<std::string> keys;
    if (!resultVectorMap.size()) {
        return std::vector<std::string>();
    }
    ResultMap& resultMap = resultVectorMap[0];
    for(std::map<std::string,std::string>::iterator it = resultMap.begin(); it != resultMap.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

}
}
