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
#include "keto/wavm_common/ParentForkGateway.hpp"

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
        //KETO_LOG_ERROR << "[WavmSessionTransaction::WavmSessionTransaction] The model size : " <<
        //    sandboxCommandMessage.model().size();
        keto::asn1::AnyHelper anyHelper(sandboxCommandMessage.model());
        RDFModel_t* rdfModel;
        TransactionMessage_t* transactionMessage;
        if ((rdfModel = anyHelper.extract<RDFModel_t>(&asn_DEF_RDFModel)) != NULL) {
            //KETO_LOG_ERROR << "[WavmSessionTransaction::WavmSessionTransaction] Load the RDF model " << sandboxCommandMessage.contract_name();
            //KETO_LOG_ERROR << "[WavmSessionTransaction::WavmSessionTransaction] Deserialize the any [" << Botan::hex_encode((uint8_t*)sandboxCommandMessage.model().c_str(),
            //                                                                        sandboxCommandMessage.model().size()) << "]";
            keto::asn1::RDFModelHelper rdfModelHelper(rdfModel);
            for (keto::asn1::RDFSubjectHelperPtr subject : rdfModelHelper.getSubjects()) {
                //KETO_LOG_ERROR << "[WavmSessionTransaction::WavmSessionTransaction] load the subject : " << subject->getSubject();
                rdfSessionPtr->persist(subject);
            }
            for (keto::asn1::RDFNtGroupHelperPtr group : rdfModelHelper.getRDFNtGroups()) {
                //KETO_LOG_ERROR << "[WavmSessionTransaction::WavmSessionTransaction] add a new group of nodes";
                rdfSessionPtr->persist(group);
            }
        } else if ((transactionMessage = anyHelper.extract<TransactionMessage_t>(&asn_DEF_TransactionMessage)) != NULL) {
            keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr(
                    new keto::transaction_common::TransactionMessageHelper(transactionMessage));
            addTransaction(transactionMessageHelperPtr,true);
        } else {
            KETO_LOG_ERROR << "Failed to deserialize the data [" << Botan::hex_encode((uint8_t*)sandboxCommandMessage.model().c_str(),
                                                                                    sandboxCommandMessage.model().size()) << "]";
        }
    } else {
        KETO_LOG_ERROR << "[WavmSessionTransaction::WavmSessionTransaction]The model is empty for the contract : " << sandboxCommandMessage.contract_name();
    }
    addTransaction(transactionMessageHelperPtr,false);
    this->contractHash = keto::asn1::HashHelper(this->sandboxCommandMessage.contract_hash());

}

WavmSessionTransaction::~WavmSessionTransaction() {
    
}

std::string WavmSessionTransaction::getSessionType() {
    return Constants::SESSION_TYPES::TRANSACTION;
}

std::string WavmSessionTransaction::getContractName() {
    return this->sandboxCommandMessage.contract_name();
}

std::string WavmSessionTransaction::getContractHash() {
    return keto::asn1::HashHelper(this->sandboxCommandMessage.contract_hash()).getHash(keto::common::StringEncoding::HEX);
}

std::string WavmSessionTransaction::getContractOwner() {
    return keto::asn1::HashHelper(this->sandboxCommandMessage.contract_owner()).getHash(keto::common::StringEncoding::HEX);
}

// the contract facade methods
std::string WavmSessionTransaction::getFeeAccount() {
    return keto::server_common::ServerInfo::getInstance()->getFeeAccountHashHex();
}

// the contract facade methods
std::string WavmSessionTransaction::getAccount() {
    return getCurrentAccountHash().getHash(keto::common::StringEncoding::HEX);
}

std::string WavmSessionTransaction::getDebitAccount() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getSourceAccount().getHash(keto::common::StringEncoding::HEX);
}

std::string WavmSessionTransaction::getCreditAccount() {
    return transactionMessageHelperPtr->getTransactionWrapper()->getTargetAccount().getHash(keto::common::StringEncoding::HEX);
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


bool WavmSessionTransaction::createDebitEntry(const std::string& accountId, const std::string& name, const std::string& description, const std::string& accountModel, const std::string& transactionValueModel,
        const keto::asn1::NumberHelper& value) {
    keto::wavm_common::RDFURLUtils accountUrl(accountModel);
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string id = this->getTransaction();
    std::string hash = accountId;
    time_t transactionTime = time(0);

    std::stringstream ss;
    ss << "debit_" << generateRandomNumber() << accountId << name << description << accountModel << transactionValueModel << transactionTime << (long)value <<  "" << id;
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
            subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::CONTRACT),
            this->getContractHash());
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::ACCOUNT_HASH),
            hash);
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE),
            value.operator long());

    return true;
}

bool WavmSessionTransaction::createCreditEntry(const std::string& accountId, const std::string& name, const std::string& description, const std::string& accountModel, const std::string& transactionValueModel,
        const keto::asn1::NumberHelper& value) {
    keto::wavm_common::RDFURLUtils accountUrl(accountModel);
    keto::wavm_common::RDFURLUtils transactionUrl(transactionValueModel);
    std::string id = this->getTransaction();
    std::string hash = accountId;
    time_t transactionTime = time(0);

    if (!isSystemContract(Constants::SYSTEM_NON_BALANCING_CONTRACTS)) {
        long balance = getBalance(transactionValueModel);
        if (balance < (long)value) {
            std::stringstream msg;
            KETO_LOG_ERROR << "The contract balance is invalid [" << this->getContractName() << "] balance [" << balance << "] value [" << (long)value << "]";
            return false;
        }
    }

    std::stringstream ss;
    ss << "credit_" << generateRandomNumber() << accountId << name << description << accountModel << transactionValueModel << transactionTime << (long)value <<  "" << id;
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
            subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::CONTRACT),
            this->getContractHash());
    this->addModelEntry(
        subjectUrl,transactionUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE),
            value.operator long());

    return true;
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
        keto::account_query::AccountSparqlQueryHelper accountSparqlQueryHelper(
                keto::server_common::Events::DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
                getCurrentAccountHash(),query);
        return addResultVectorMap(accountSparqlQueryHelper.processResult(
                ParentForkGateway::processEvent(accountSparqlQueryHelper.generateEvent())));
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
    if (isSystemContract()) {
        return;
    }
    if (subjectUrl.find(this->sandboxCommandMessage.contract_namespace()) == 0) {
        return;
    }
    // exclude the blockchain currency
    if (subjectUrl.find(Constants::BLOCKCHAIN_CURRENCY_NAMESPACE) == 0) {
        return;
    }
    /*
    the original logic relied on using the contract name or contract hash to uniquely identify it. this has been
    changed to namespace
    if (subjectUrl.find(this->sandboxCommandMessage.contract_name()) != std::string::npos) {
        return;
    }
    if (subjectUrl.find(this->contractHash.getHash(keto::common::StringEncoding::HEX)) != std::string::npos) {
        return;
    }*/
    KETO_LOG_ERROR << "[WavmSessionTransaction::validateSubject] The subject is invalid ["
        << subjectUrl << "][" << subjectUrl.find(Constants::BLOCKCHAIN_CURRENCY_NAMESPACE) <<"]";
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
    try {
        keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
        keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr, predicateUrl);

        keto::asn1::RDFObjectHelper objectHelper;
        objectHelper.setDataType(RDFConstants::TYPES::STRING).
                setType(RDFConstants::NODE_TYPES::LITERAL).
                setValue(value);

        predicate->addObject(objectHelper);

        rdfSessionPtr->setStringValue(
                subjectUrl, predicateUrl, value);

    } catch (...) {
        KETO_LOG_ERROR << "Ignore model request [" << subjectUrl << "][" << predicateUrl << "] it is invalid";
    }
}

void WavmSessionTransaction::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const long value) {
    try {
        keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
        keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr, predicateUrl);

        std::stringstream ss;
        ss << value;
        keto::asn1::RDFObjectHelper objectHelper;
        objectHelper.setDataType(RDFConstants::TYPES::LONG).
                setType(RDFConstants::NODE_TYPES::LITERAL).
                setValue(ss.str());

        predicate->addObject(objectHelper);

        rdfSessionPtr->setLongValue(
                subjectUrl, predicateUrl, value);
    } catch (...) {
        KETO_LOG_ERROR << "Ignore model request [" << subjectUrl << "][" << predicateUrl << "] it is invalid";
    }
}

void WavmSessionTransaction::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const float value) {
    try {
        keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
        keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr, predicateUrl);

        std::stringstream ss;
        ss << value;
        keto::asn1::RDFObjectHelper objectHelper;
        objectHelper.setDataType(RDFConstants::TYPES::FLOAT).
                setType(RDFConstants::NODE_TYPES::LITERAL).
                setValue(ss.str());

        predicate->addObject(objectHelper);

        rdfSessionPtr->setFloatValue(
                subjectUrl, predicateUrl, value);
    } catch (...) {
        KETO_LOG_ERROR << "Ignore model request [" << subjectUrl << "][" << predicateUrl << "] it is invalid";
    }
}

void WavmSessionTransaction::addBooleanModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const bool value) {
    try {
        keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
        keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr, predicateUrl);

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
                subjectUrl, predicateUrl, value);
    } catch (...) {
        KETO_LOG_ERROR << "Ignore model request [" << subjectUrl << "][" << predicateUrl << "] it is invalid";
    }
}

void WavmSessionTransaction::addDateTimeModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
        const time_t value) {
    try {
        keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
        keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr, predicateUrl);

        keto::asn1::RDFObjectHelper objectHelper;
        objectHelper.setDataType(RDFConstants::TYPES::DATE_TIME).
                setType(RDFConstants::NODE_TYPES::LITERAL).
                setValue(keto::server_common::RDFUtils::convertTimeToRDFDateTime(value));
        predicate->addObject(objectHelper);

        rdfSessionPtr->setDateTimeValue(
                subjectUrl, predicateUrl, value);
    } catch (...) {
        KETO_LOG_ERROR << "Ignore model request [" << subjectUrl << "][" << predicateUrl << "] it is invalid";
    }
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

long WavmSessionTransaction::getBalance(const std::string& transactionValueModel) {
    // validate the the credit entry can be added
    keto::wavm_common::RDFURLUtils predicateBuilderUrl(transactionValueModel);
    std::stringstream ssQuery;
    ssQuery << "SELECT ?type ( SUM( ?value ) AS ?totalValue )" <<
            "        WHERE { " <<
            "?transaction <" << predicateBuilderUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::CONTRACT) << "> \"" <<  this->getContractHash() << "\"^^<http://www.w3.org/2001/XMLSchema#string> . " <<
            "?transaction <" << predicateBuilderUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::TYPE) << "> ?type . " <<
            "?transaction <" << predicateBuilderUrl.buildPredicateUrl(RDFConstants::ACCOUNT_TRANSACTION_PREDICATES::VALUE) << "> ?value . " <<
            "        } GROUP BY ?type";

    ResultVectorMap resultVectorMap = this->rdfSessionPtr->executeQuery(ssQuery.str());
    long debitVal = 0;
    long creditVal = 0;
    for (ResultMap resultMap : resultVectorMap) {
        KETO_LOG_ERROR << "[WavmSessionTransaction::getBalance] entry [" << resultMap["type"] << "] value [" << resultMap["totalValue"] << "]";
        if (resultMap["type"] == keto::server_common::Constants::Constants::ACCOUNT_ACTIONS::CREDIT) {
            creditVal += std::stol(resultMap["totalValue"]);
        } else if (resultMap["type"] == keto::server_common::Constants::Constants::ACCOUNT_ACTIONS::DEBIT) {
            debitVal += std::stol(resultMap["totalValue"]);
        }
    }
    KETO_LOG_ERROR << "[WavmSessionTransaction::getBalance] debit [" << debitVal << "] credit [" << creditVal << "]";
    return debitVal - creditVal;
}

bool WavmSessionTransaction::isSystemContract(const std::vector<const char*>& contracts) {
    for (const char* contractName : contracts) {
        if (this->sandboxCommandMessage.contract_name() == contractName){
            return true;
        }
    }
    return false;
}

int WavmSessionTransaction::generateRandomNumber() {
    // setup srand and get a random integer
    unsigned int seedp = time(0);
    int randomNumber = rand_r(&seedp);
    return randomNumber;
}

}
}
