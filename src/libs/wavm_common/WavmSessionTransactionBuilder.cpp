//
// Created by Brett Chaldecott on 2019-10-07.
//

#include <cctype>

#include "keto/wavm_common/WavmSessionTransactionBuilder.hpp"
#include "keto/wavm_common/Constants.hpp"
#include "keto/wavm_common/RDFConstants.hpp"
#include "keto/wavm_common/Exception.hpp"

#include "keto/chain_common/SignedTransactionBuilder.hpp"

#include "keto/transaction_common/TransactionTraceBuilder.hpp"

#include "keto/server_common/RDFUtils.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/ServerInfo.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/key_store_utils/Events.hpp"

#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

#include "keto/wavm_common/ParentForkGateway.hpp"

namespace keto {
namespace wavm_common {


std::string WavmSessionTransactionBuilder::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

WavmSessionTransactionBuilder::WavmSessionModelBuilder::WavmSessionModelBuilder() {

}

WavmSessionTransactionBuilder::WavmSessionModelBuilder::~WavmSessionModelBuilder() {

}

WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::WavmSessionRDFModelBuilder() {

}

WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::~WavmSessionRDFModelBuilder() {

}

std::string WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getRequestStringValue(const std::string& subjectUrl, const std::string& predicateUrl) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    return predicate->getStringLiteral();
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::setRequestStringValue(const std::string& subjectUrl, const std::string& predicateUrl, const std::string& value) {
    this->addModelEntry(subjectUrl,predicateUrl,value);
}

long WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getRequestLongValue(const std::string& subjectUrl, const std::string& predicateUrl) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    return predicate->getLongLiteral();
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::setRequestLongValue(const std::string& subjectUrl, const std::string& predicateUrl, const long& value) {
    this->addModelEntry(subjectUrl,predicateUrl,value);
}

float WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getRequestFloatValue(const std::string& subjectUrl, const std::string& predicateUrl) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    return predicate->getLongLiteral();
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::setRequestFloatValue(const std::string& subjectUrl, const std::string& predicateUrl, const float& value) {
    this->addModelEntry(subjectUrl,predicateUrl,value);
}

bool WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getRequestBooleanValue(const std::string& subjectUrl, const std::string& predicateUrl) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);
    return keto::server_common::StringUtils(predicate->getStringLiteral()).isIEqual("true");
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::setRequestBooleanValue(const std::string& subject, const std::string& predicate, const bool& value) {
    this->addBooleanModelEntry(subject,predicate,value);
}

std::string WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getType() {
    return Constants::TRANSACTION_BUILDER::MODEL::RDF;
}

WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::operator keto::asn1::AnyHelper() const {
    return this->modelHelper;
}

keto::asn1::RDFSubjectHelperPtr WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getSubject(const std::string& subjectUrl) {
    if (!this->modelHelper.contains(subjectUrl)) {
        keto::asn1::RDFSubjectHelper subject(subjectUrl);
        this->modelHelper.addSubject(subject);
    }
    return this->modelHelper[subjectUrl];
}

keto::asn1::RDFPredicateHelperPtr WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::getPredicate(
        keto::asn1::RDFSubjectHelperPtr subject, const std::string& predicateUrl) {
    if (!subject->containsPredicate(predicateUrl)) {
        keto::asn1::RDFPredicateHelper predicate(predicateUrl);
        subject->addPredicate(predicate);
    }
    return (*subject)[predicateUrl];

}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                   const std::string& value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);

    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::STRING).
            setType(RDFConstants::NODE_TYPES::LITERAL).
            setValue(value);

    predicate->addObject(objectHelper);
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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
}
void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::addBooleanModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
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
}

void WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder::addDateTimeModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                           const time_t value) {
    keto::asn1::RDFSubjectHelperPtr subjectHelperPtr = getSubject(subjectUrl);
    keto::asn1::RDFPredicateHelperPtr predicate = getPredicate(subjectHelperPtr,predicateUrl);

    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(RDFConstants::TYPES::DATE_TIME).
            setType(RDFConstants::NODE_TYPES::LITERAL).
            setValue(keto::server_common::RDFUtils::convertTimeToRDFDateTime(value));
    predicate->addObject(objectHelper);

}

WavmSessionTransactionBuilder::WavmSessionActionBuilder::WavmSessionActionBuilder(int id, const std::string& modelType) : id(id) {
    if (modelType == Constants::TRANSACTION_BUILDER::MODEL::RDF) {
        this->wavmSessionModelBuilderPtr = WavmSessionModelBuilderPtr(new WavmSessionRDFModelBuilder());
    } else {
        BOOST_THROW_EXCEPTION(keto::wavm_common::UnsupportedModelType());
    }
    this->actionBuilderPtr = keto::chain_common::ActionBuilder::createAction();
}

WavmSessionTransactionBuilder::WavmSessionActionBuilder::~WavmSessionActionBuilder() {

}

// this id is only used for tracking the actions created for the
int WavmSessionTransactionBuilder::WavmSessionActionBuilder::getId() {
    return this->id;
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::WavmSessionActionBuilder::getContract() {
    return this->actionBuilderPtr->getContract();
}

void WavmSessionTransactionBuilder::WavmSessionActionBuilder::setContract(const keto::asn1::HashHelper& contract) {
    this->actionBuilderPtr->setContract(contract);
}

std::string WavmSessionTransactionBuilder::WavmSessionActionBuilder::getContractName() {
    return this->actionBuilderPtr->getContractName();
}

void WavmSessionTransactionBuilder::WavmSessionActionBuilder::setContractName(const std::string& contractName) {
    this->actionBuilderPtr->setContractName(contractName);
}

std::string WavmSessionTransactionBuilder::WavmSessionActionBuilder::getModelType() {
    return this->wavmSessionModelBuilderPtr->getType();
}

WavmSessionTransactionBuilder::WavmSessionModelBuilderPtr WavmSessionTransactionBuilder::WavmSessionActionBuilder::getModel() {
    return this->wavmSessionModelBuilderPtr;
}

WavmSessionTransactionBuilder::WavmSessionActionBuilder::operator keto::chain_common::ActionBuilderPtr() const {
    if (this->wavmSessionModelBuilderPtr->getType() == keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
        keto::asn1::AnyHelper anyHelper = *std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(this->wavmSessionModelBuilderPtr);
        this->actionBuilderPtr->setModel(anyHelper);
    } else {
        KETO_LOG_ERROR << "[WavmSessionTransactionBuilder::WavmSessionActionBuilder] Unsupported model type : " << this->wavmSessionModelBuilderPtr->getType();
    }

    return this->actionBuilderPtr;
}



WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::WavmNestedTransactionBuilder(int id, bool encrypted, const keto::crypto::KeyLoaderPtr& keyLoaderPtr)
: id(id), encrypted(encrypted), keyLoaderPtr(keyLoaderPtr) {
    this->transactionBuilderPtr = keto::chain_common::TransactionBuilder::createTransaction();
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::WavmNestedTransactionBuilder(int id, bool encrypted, const keto::asn1::HashHelper& hashHelper,
                             const keto::crypto::KeyLoaderPtr& keyLoaderPtr)
: id(id), encrypted(encrypted), keyLoaderPtr(keyLoaderPtr) {
    this->transactionBuilderPtr = keto::chain_common::TransactionBuilder::createTransaction();
    this->transactionBuilderPtr->setParent(hashHelper);
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::~WavmNestedTransactionBuilder() {

}

int WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getId() {
    return id;
}

void WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::setValue(const keto::asn1::NumberHelper& numberHelper) {
    this->transactionBuilderPtr->setValue(numberHelper);
}

keto::asn1::NumberHelper WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getValue() {
    return this->transactionBuilderPtr->getValue();
}

void WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::setParent(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setParent(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getParent() {
    return this->transactionBuilderPtr->getParent();
}

void WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::setSourceAccount(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setSourceAccount(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getSourceAccount() {
    return this->transactionBuilderPtr->getSourceAccount();
}

void WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::setTargetAccount(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setTargetAccount(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getTargetAccount() {
    return this->transactionBuilderPtr->getTargetAccount();
}

void WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::setTransactionSignator(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setTransactionSignator(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getTransactionSignator() {
    return this->transactionBuilderPtr->getTransactionSignator();
}

void WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::setCreatorId(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setCreatorId(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getCreatorId() {
    return this->transactionBuilderPtr->getCreatorId();
}

WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::createAction(const std::string& modelType) {
    int id = this->actions.size();
    WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr
            wavmSessionActionBuilderPtr(new WavmSessionTransactionBuilder::WavmSessionActionBuilder(id,modelType));
    this->actions.push_back(wavmSessionActionBuilderPtr);
    return wavmSessionActionBuilderPtr;
}

WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getAction(const int& id) {
    if (id >= this->actions.size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionIdForThisSession());
    }
    return this->actions[id];
}


WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::createNested(bool encrypted) {
    int id = this->nested.size();
    WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr
            wavmNestedTransactionBuilderPtr(new WavmSessionTransactionBuilder::WavmNestedTransactionBuilder(id,encrypted,this->keyLoaderPtr));
    this->nested.push_back(wavmNestedTransactionBuilderPtr);
    return wavmNestedTransactionBuilderPtr;
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::createNested(bool encrypted, const keto::asn1::HashHelper& hashHelper) {
    int id = this->nested.size();
    WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr
            wavmNestedTransactionBuilderPtr(new WavmSessionTransactionBuilder::WavmNestedTransactionBuilder(id,encrypted, hashHelper,this->keyLoaderPtr));
    this->nested.push_back(wavmNestedTransactionBuilderPtr);
    return wavmNestedTransactionBuilderPtr;
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getNested(const int& id) {
    if (id >= this->actions.size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidNestedTransactionIdForThisSession());
    }
    return this->nested[id];
}

keto::transaction_common::TransactionMessageHelperPtr WavmSessionTransactionBuilder::WavmNestedTransactionBuilder::getTransaction() {
    this->transactionBuilderPtr->setEncrypted(this->encrypted);
    std::shared_ptr<keto::chain_common::SignedTransactionBuilder> signedTransBuild =
            keto::chain_common::SignedTransactionBuilder::createTransaction(
                    this->keyLoaderPtr);

    for (WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr action: this->actions) {
        this->transactionBuilderPtr->addAction(*action);
    }

    signedTransBuild->setTransaction(this->transactionBuilderPtr);
    signedTransBuild->sign();

    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr(
            new keto::transaction_common::TransactionWrapperHelper(signedTransBuild));

    transactionWrapperHelperPtr->addTransactionTrace(
            *keto::transaction_common::TransactionTraceBuilder::createTransactionTrace(
                    keto::server_common::ServerInfo::getInstance()->getAccountHash(), this->keyLoaderPtr));

    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            keto::transaction_common::TransactionMessageHelperPtr(
                    new keto::transaction_common::TransactionMessageHelper(
                            transactionWrapperHelperPtr));

    for (WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr nestedTransacction: this->nested) {
        transactionMessageHelperPtr->addNestedTransaction(nestedTransacction->getTransaction());
    }
    return transactionMessageHelperPtr;
}


WavmSessionTransactionBuilder::WavmSessionTransactionBuilder(int id, const keto::crypto::KeyLoaderPtr& keyLoaderPtr)
        : id(id), keyLoaderPtr(keyLoaderPtr), submitted(false) {
    this->transactionBuilderPtr = keto::chain_common::TransactionBuilder::createTransaction();
}

WavmSessionTransactionBuilder::WavmSessionTransactionBuilder(int id, const keto::asn1::HashHelper& hashHelper,
                                                             const keto::crypto::KeyLoaderPtr& keyLoaderPtr)
        : id(id), keyLoaderPtr(keyLoaderPtr), submitted(false) {
    this->transactionBuilderPtr = keto::chain_common::TransactionBuilder::createTransaction();
    this->transactionBuilderPtr->setParent(hashHelper);
}


WavmSessionTransactionBuilder::~WavmSessionTransactionBuilder() {

}

int WavmSessionTransactionBuilder::getId() {
    return this->id;
}


void WavmSessionTransactionBuilder::setValue(const keto::asn1::NumberHelper& numberHelper) {
    this->transactionBuilderPtr->setValue(numberHelper);
}

keto::asn1::NumberHelper WavmSessionTransactionBuilder::getValue() {
    return this->transactionBuilderPtr->getValue();
}

void WavmSessionTransactionBuilder::setParent(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setParent(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::getParent() {
    return this->transactionBuilderPtr->getParent();
}

void WavmSessionTransactionBuilder::setSourceAccount(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setSourceAccount(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::getSourceAccount() {
    return this->transactionBuilderPtr->getSourceAccount();
}

void WavmSessionTransactionBuilder::setTargetAccount(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setTargetAccount(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::getTargetAccount() {
    return this->transactionBuilderPtr->getTargetAccount();
}

void WavmSessionTransactionBuilder::setTransactionSignator(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setTransactionSignator(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::getTransactionSignator() {
    return this->transactionBuilderPtr->getTransactionSignator();
}

void WavmSessionTransactionBuilder::setCreatorId(const keto::asn1::HashHelper& hashHelper) {
    this->transactionBuilderPtr->setCreatorId(hashHelper);
}

keto::asn1::HashHelper WavmSessionTransactionBuilder::getCreatorId() {
    return this->transactionBuilderPtr->getCreatorId();
}


WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr WavmSessionTransactionBuilder::createAction(const std::string& modelType) {
    int id = this->actions.size();
    WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr
        wavmSessionActionBuilderPtr(new WavmSessionTransactionBuilder::WavmSessionActionBuilder(id,modelType));
    this->actions.push_back(wavmSessionActionBuilderPtr);
    return wavmSessionActionBuilderPtr;
}

WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr WavmSessionTransactionBuilder::getAction(const int& id) {
    if (id >= this->actions.size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionIdForThisSession());
    }
    return this->actions[id];
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr WavmSessionTransactionBuilder::createNested(bool encrypted) {
    int id = this->nested.size();
    WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr
            wavmNestedTransactionBuilderPtr(new WavmSessionTransactionBuilder::WavmNestedTransactionBuilder(id,encrypted,this->keyLoaderPtr));
    this->nested.push_back(wavmNestedTransactionBuilderPtr);
    return wavmNestedTransactionBuilderPtr;
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr WavmSessionTransactionBuilder::createNested(bool encrypted, const keto::asn1::HashHelper& hashHelper) {
    int id = this->nested.size();
    WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr
            wavmNestedTransactionBuilderPtr(new WavmSessionTransactionBuilder::WavmNestedTransactionBuilder(id,encrypted, hashHelper,this->keyLoaderPtr));
    this->nested.push_back(wavmNestedTransactionBuilderPtr);
    return wavmNestedTransactionBuilderPtr;
}

WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr WavmSessionTransactionBuilder::getNested(const int& id) {
    if (id >= this->actions.size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidNestedTransactionIdForThisSession());
    }
    return this->nested[id];
}


void WavmSessionTransactionBuilder::submit() {
    return this->submitWithStatus("INIT");
}

void WavmSessionTransactionBuilder::submitWithStatus(const std::string& status) {

    KETO_LOG_INFO << "[WavmSessionTransactionBuilder::submitWithStatus] submit with status : " << status;
    if (this->submitted) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::TransactionAlreadySubmitted());
    }
    std::shared_ptr<keto::chain_common::SignedTransactionBuilder> signedTransBuild =
            keto::chain_common::SignedTransactionBuilder::createTransaction(
                    this->keyLoaderPtr);

    for (WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr action: this->actions) {
        this->transactionBuilderPtr->addAction(*action);
    }

    signedTransBuild->setTransaction(this->transactionBuilderPtr);
    signedTransBuild->sign();

    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr(
            new keto::transaction_common::TransactionWrapperHelper(signedTransBuild));

    if (status == "CREDIT") {
        transactionWrapperHelperPtr->setStatus(Status::Status_credit);
    }

    transactionWrapperHelperPtr->addTransactionTrace(
            *keto::transaction_common::TransactionTraceBuilder::createTransactionTrace(
                    keto::server_common::ServerInfo::getInstance()->getAccountHash(), this->keyLoaderPtr));

    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            keto::transaction_common::TransactionMessageHelperPtr(
            new keto::transaction_common::TransactionMessageHelper(
                    transactionWrapperHelperPtr));

    for (WavmSessionTransactionBuilder::WavmNestedTransactionBuilderPtr nestedTransacction: this->nested) {
        transactionMessageHelperPtr->addNestedTransaction(nestedTransacction->getTransaction());
    }

    keto::transaction_common::TransactionProtoHelper
            transactionProtoHelper(transactionMessageHelperPtr);


    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper;
    messageWrapperProtoHelper.setTransaction(transactionProtoHelper);

    KETO_LOG_INFO << "[WavmSessionTransactionBuilder::submitWithStatus] Encrypt the transaction";
    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    messageWrapper = keto::server_common::fromEvent<keto::proto::MessageWrapper>(
            ParentForkGateway::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                    keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION,messageWrapper)));

    KETO_LOG_INFO << "[WavmSessionTransactionBuilder::submitWithStatus] Queue the message";
    keto::proto::MessageWrapperResponse  messageWrapperResponse =
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                    ParentForkGateway::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                            keto::server_common::Events::QUEUE_MESSAGE,messageWrapper)));

    KETO_LOG_INFO << "[WavmSessionTransactionBuilder::submitWithStatus] Failed to queue message";

}

}
}
