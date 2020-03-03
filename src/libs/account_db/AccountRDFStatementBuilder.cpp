/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountStatementBuilder.cpp
 * Author: ubuntu
 * 
 * Created on March 20, 2018, 11:54 AM
 */

#include <vector>
#include <sstream>
#include <ChangeSet.h>

#include "TransactionMessage.h"

#include "keto/account_db/AccountRDFStatementBuilder.hpp"
#include "keto/account_db/Exception.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/account_db/AccountSystemOntologyTypes.hpp"
#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/asn1/Constants.hpp"
#include "keto/server_common/RDFUtils.hpp"

namespace keto {
namespace account_db {

std::string AccountRDFStatementBuilder::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountRDFStatementBuilder::AccountRDFStatementBuilder(
        const keto::asn1::HashHelper& chainId,
        const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr,
        bool existingAccount) :
        newChain(false),
        chainId(chainId),
        transactionWrapperHelperPtr(transactionWrapperHelperPtr) {

    this->accountInfo.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);

    keto::asn1::HashHelper accountHash = transactionWrapperHelperPtr->getCurrentAccount();

    // build the block chain rdf statement
    this->rdfModelHelperPtr = keto::asn1::RDFModelHelperPtr(new keto::asn1::RDFModelHelper());


    // build the transaction statement
    keto::asn1::RDFSubjectHelper transactionRDFSubject(buildRdfUri(AccountSystemOntologyTypes::TRANSACTION_ONTOLOGY_CLASS,
                                                                   transactionWrapperHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX)));
    keto::asn1::RDFPredicateHelper transactionIdPredicate = buildPredicate(AccountSystemOntologyTypes::TRANSACTION_PREDICATES::ID,
                                                                           transactionWrapperHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX),
                                                                           keto::asn1::Constants::RDF_TYPES::STRING,
                                                                           keto::asn1::Constants::RDF_NODE::LITERAL);
    transactionRDFSubject.addPredicate(transactionIdPredicate);
    keto::asn1::RDFPredicateHelper transactionDateRDFPredicate = buildPredicate(AccountSystemOntologyTypes::TRANSACTION_PREDICATES::DATE,
                                                                                keto::server_common::RDFUtils::convertTimeToRDFDateTime(transactionWrapperHelperPtr->getSignedTransaction()->getTransaction()->getDate()),
                                                                                keto::asn1::Constants::RDF_TYPES::DATE_TIME,
                                                                                keto::asn1::Constants::RDF_NODE::LITERAL);
    transactionRDFSubject.addPredicate(transactionDateRDFPredicate);

    this->rdfModelHelperPtr->addSubject(transactionRDFSubject);

    // add the statement to the beginning of the list
    statements.push_back(AccountRDFStatementPtr(new AccountRDFStatement(this->rdfModelHelperPtr)));

    for (int count = 0; count <
                        transactionWrapperHelperPtr->operator TransactionWrapper_t&().changeSet.list.count; count++) {
        SignedChangeSet* signedChangeSet =
                transactionWrapperHelperPtr->operator TransactionWrapper_t&().changeSet.list.array[count];
        for (int index = 0 ; index < signedChangeSet->changeSet.changes.list.count; index++) {
            // ignore changes not associated with the current transaction status
            if (signedChangeSet->changeSet.status != transactionWrapperHelperPtr->getStatus()) {
                continue;
            }
            AccountRDFStatementPtr accountRDFStatement(new AccountRDFStatement(
                    signedChangeSet->changeSet.changes.list.array[index]));

            for (std::string subject : accountRDFStatement->getModel()->subjects()) {
                keto::asn1::RDFSubjectHelperPtr subjectPtr =
                        accountRDFStatement->getModel()->operator [](subject);

                // add the account information
                keto::asn1::RDFPredicateHelper transactionAccountRDFPredicate =
                        buildPredicate(AccountSystemOntologyTypes::ACCOUNT_PREDICATES::TRANSACTION,
                                       buildRdfUri(AccountSystemOntologyTypes::TRANSACTION_ONTOLOGY_CLASS,
                                                   transactionWrapperHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX)),
                                       keto::asn1::Constants::RDF_TYPES::STRING,
                                       keto::asn1::Constants::RDF_NODE::URI);
                subjectPtr->addPredicate(transactionAccountRDFPredicate);

                // add the owner information
                keto::asn1::RDFPredicateHelper transactionOwnerRDFPredicate =
                        buildPredicate(AccountSystemOntologyTypes::ACCOUNT_PERMISSIONS::OWNER,
                                       buildRdfUri(AccountSystemOntologyTypes::ACCOUNT_ONTOLOGY_CLASS,
                                                   accountHash.getHash(keto::common::StringEncoding::HEX)),
                                       keto::asn1::Constants::RDF_TYPES::STRING,
                                       keto::asn1::Constants::RDF_NODE::URI);
                subjectPtr->addPredicate(transactionOwnerRDFPredicate);

                // add the group information
                keto::asn1::RDFPredicateHelper transactionGroupRDFPredicate =
                        buildPredicate(AccountSystemOntologyTypes::ACCOUNT_PERMISSIONS::GROUP,
                                       buildRdfUri(AccountSystemOntologyTypes::GROUP_ONTOLOGY_CLASS,
                                                   accountHash.getHash(keto::common::StringEncoding::HEX)),
                                       keto::asn1::Constants::RDF_TYPES::STRING,
                                       keto::asn1::Constants::RDF_NODE::URI);
                subjectPtr->addPredicate(transactionGroupRDFPredicate);

                if (!AccountSystemOntologyTypes::validateClassOperation(
                        accountHash,existingAccount,subjectPtr)) {
                    std::stringstream  ss;
                    ss << "Invalid operation on account [" << accountHash.getHash(keto::common::StringEncoding::HEX) << "]["
                        << existingAccount << "][" << subjectPtr->getSubject() << "]";
                    BOOST_THROW_EXCEPTION(keto::account_db::InvalidAccountOperationException(ss.str()));
                }
                if (AccountSystemOntologyTypes::isAccountOntologyClass(subjectPtr)) {
                    this->action =
                            (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::STATUS]->getStringLiteral();

                    // setup the account hash
                    keto::asn1::HashHelper accountHash(
                            (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::ID]->getStringLiteral(),
                            keto::common::HEX);
                    accountInfo.set_account_hash(
                            keto::crypto::SecureVectorUtils().copySecureToString(
                                    accountHash));

                    if (subjectPtr->containsPredicate(AccountSystemOntologyTypes::ACCOUNT_PREDICATES::PARENT)) {
                        keto::asn1::HashHelper parentAccountHash(
                                (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::PARENT]->getStringLiteral(),
                                keto::common::HEX);
                        accountInfo.set_parent_account_hash(
                                keto::crypto::SecureVectorUtils().copySecureToString(
                                        parentAccountHash));
                    }
                    accountInfo.set_account_type(
                            (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::TYPE]->getStringLiteral());

                }
            }
            statements.push_back(accountRDFStatement);

        }
    }
}

    
AccountRDFStatementBuilder::AccountRDFStatementBuilder(
        const keto::asn1::HashHelper& chainId,
        const keto::asn1::HashHelper& blockId,
        const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr,
        bool existingAccount) :
        newChain(false),
        chainId(chainId),
        blockId(blockId),
        transactionWrapperHelperPtr(transactionWrapperHelperPtr) {

    this->accountInfo.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);

    keto::asn1::HashHelper accountHash = transactionWrapperHelperPtr->getCurrentAccount();

    // build the block chain rdf statement
    this->rdfModelHelperPtr = keto::asn1::RDFModelHelperPtr(new keto::asn1::RDFModelHelper());

    // if block id is not set this will be the case in a dirty write
    keto::asn1::RDFSubjectHelper blockRDFSubject(buildRdfUri(AccountSystemOntologyTypes::BLOCK_ONTOLOGY_CLASS,
                                                             blockId.getHash(keto::common::StringEncoding::HEX)));
    keto::asn1::RDFPredicateHelper blockIdPredicate = buildPredicate(AccountSystemOntologyTypes::BLOCK_PREDICATES::ID,
                                                                 blockId.getHash(keto::common::StringEncoding::HEX),
                                                                 keto::asn1::Constants::RDF_TYPES::STRING,
                                                                 keto::asn1::Constants::RDF_NODE::LITERAL);
    blockRDFSubject.addPredicate(blockIdPredicate);
    keto::asn1::RDFPredicateHelper chainRDFPredicate = buildPredicate(AccountSystemOntologyTypes::BLOCK_PREDICATES::CHAIN,
                                                                      buildRdfUri(AccountSystemOntologyTypes::CHAIN_ONTOLOGY_CLASS,
                                                                                  chainId.getHash(keto::common::StringEncoding::HEX)),
                                                                      keto::asn1::Constants::RDF_TYPES::STRING,
                                                                      keto::asn1::Constants::RDF_NODE::URI);
    blockRDFSubject.addPredicate(chainRDFPredicate);

    this->rdfModelHelperPtr->addSubject(blockRDFSubject);


    // build the transaction statement
    keto::asn1::RDFSubjectHelper transactionRDFSubject(buildRdfUri(AccountSystemOntologyTypes::TRANSACTION_ONTOLOGY_CLASS,
            transactionWrapperHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX)));
    keto::asn1::RDFPredicateHelper transactionIdPredicate = buildPredicate(AccountSystemOntologyTypes::TRANSACTION_PREDICATES::ID,
            transactionWrapperHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX),
            keto::asn1::Constants::RDF_TYPES::STRING,
            keto::asn1::Constants::RDF_NODE::LITERAL);
    transactionRDFSubject.addPredicate(transactionIdPredicate);
    keto::asn1::RDFPredicateHelper transactionBlockRDFPredicate = buildPredicate(
            AccountSystemOntologyTypes::TRANSACTION_PREDICATES::BLOCK,
            buildRdfUri(AccountSystemOntologyTypes::BLOCK_ONTOLOGY_CLASS,
                        blockId.getHash(keto::common::StringEncoding::HEX)),
            keto::asn1::Constants::RDF_TYPES::STRING,
            keto::asn1::Constants::RDF_NODE::URI);
    transactionRDFSubject.addPredicate(transactionBlockRDFPredicate);
    keto::asn1::RDFPredicateHelper transactionDateRDFPredicate = buildPredicate(AccountSystemOntologyTypes::TRANSACTION_PREDICATES::DATE,
            keto::server_common::RDFUtils::convertTimeToRDFDateTime(transactionWrapperHelperPtr->getSignedTransaction()->getTransaction()->getDate()),
            keto::asn1::Constants::RDF_TYPES::DATE_TIME,
            keto::asn1::Constants::RDF_NODE::LITERAL);
    transactionRDFSubject.addPredicate(transactionDateRDFPredicate);
    keto::asn1::RDFPredicateHelper transactionAccountRDFPredicate = buildPredicate(AccountSystemOntologyTypes::TRANSACTION_PREDICATES::ACCOUNT,
            transactionWrapperHelperPtr->getCurrentAccount().getHash(keto::common::StringEncoding::HEX),
            keto::asn1::Constants::RDF_TYPES::STRING,
            keto::asn1::Constants::RDF_NODE::LITERAL);
    transactionRDFSubject.addPredicate(transactionAccountRDFPredicate);

    this->rdfModelHelperPtr->addSubject(transactionRDFSubject);



    // add the statement to the beginning of the list
    statements.push_back(AccountRDFStatementPtr(new AccountRDFStatement(this->rdfModelHelperPtr)));
    
    for (int count = 0; count < 
            transactionWrapperHelperPtr->operator TransactionWrapper_t&().changeSet.list.count; count++) {
        SignedChangeSet* signedChangeSet =
                transactionWrapperHelperPtr->operator TransactionWrapper_t&().changeSet.list.array[count];
        for (int index = 0 ; index < signedChangeSet->changeSet.changes.list.count; index++) {
            // ignore changes not associated with the current transaction status
            if (signedChangeSet->changeSet.status != transactionWrapperHelperPtr->getStatus()) {
                continue;
            }

            AccountRDFStatementPtr accountRDFStatement(new AccountRDFStatement(
                signedChangeSet->changeSet.changes.list.array[index]));
            
            for (std::string subject : accountRDFStatement->getModel()->subjects()) {
                keto::asn1::RDFSubjectHelperPtr subjectPtr = 
                        (*accountRDFStatement->getModel())[subject];

                // add the account information
                keto::asn1::RDFPredicateHelper transactionAccountRDFPredicate =
                        buildPredicate(AccountSystemOntologyTypes::ACCOUNT_PREDICATES::TRANSACTION,
                                       buildRdfUri(AccountSystemOntologyTypes::TRANSACTION_ONTOLOGY_CLASS,
                                                   transactionWrapperHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX)),
                                       keto::asn1::Constants::RDF_TYPES::STRING,
                                       keto::asn1::Constants::RDF_NODE::URI);
                subjectPtr->addPredicate(transactionAccountRDFPredicate);

                // add the owner information
                keto::asn1::RDFPredicateHelper transactionOwnerRDFPredicate =
                        buildPredicate(AccountSystemOntologyTypes::ACCOUNT_PERMISSIONS::OWNER,
                                       buildRdfUri(AccountSystemOntologyTypes::ACCOUNT_ONTOLOGY_CLASS,
                                                   accountHash.getHash(keto::common::StringEncoding::HEX)),
                                       keto::asn1::Constants::RDF_TYPES::STRING,
                                       keto::asn1::Constants::RDF_NODE::URI);
                subjectPtr->addPredicate(transactionOwnerRDFPredicate);

                // add the group information
                keto::asn1::RDFPredicateHelper transactionGroupRDFPredicate =
                        buildPredicate(AccountSystemOntologyTypes::ACCOUNT_PERMISSIONS::GROUP,
                                       buildRdfUri(AccountSystemOntologyTypes::GROUP_ONTOLOGY_CLASS,
                                                   accountHash.getHash(keto::common::StringEncoding::HEX)),
                                       keto::asn1::Constants::RDF_TYPES::STRING,
                                       keto::asn1::Constants::RDF_NODE::URI);
                subjectPtr->addPredicate(transactionGroupRDFPredicate);

                if (!AccountSystemOntologyTypes::validateClassOperation(
                    accountHash,existingAccount,subjectPtr)) {
                    std::stringstream ss;
                    ss << "Invalid operation on account [" << accountHash.getHash(keto::common::StringEncoding::HEX) << "]["
                       << existingAccount << "][" << subjectPtr->getSubject() << "]";
                    BOOST_THROW_EXCEPTION(keto::account_db::InvalidAccountOperationException(ss.str()));
                }
                if (AccountSystemOntologyTypes::isAccountOntologyClass(subjectPtr)) {
                    this->action = 
                            (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::STATUS]->getStringLiteral();
                    
                    // setup the account hash
                    keto::asn1::HashHelper accountHash(
                            (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::ID]->getStringLiteral(),
                            keto::common::HEX);
                    accountInfo.set_account_hash(
                        keto::crypto::SecureVectorUtils().copySecureToString(
                        accountHash));
                    
                    if (subjectPtr->containsPredicate(AccountSystemOntologyTypes::ACCOUNT_PREDICATES::PARENT)) {
                        keto::asn1::HashHelper parentAccountHash(
                            (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::PARENT]->getStringLiteral(),
                                keto::common::HEX);
                        accountInfo.set_parent_account_hash(
                            keto::crypto::SecureVectorUtils().copySecureToString(
                            parentAccountHash));
                    }
                    accountInfo.set_account_type(
                        (*subjectPtr)[AccountSystemOntologyTypes::ACCOUNT_PREDICATES::TYPE]->getStringLiteral());

                }
            }
            statements.push_back(accountRDFStatement);
            
        }
    }
}

AccountRDFStatementBuilder::~AccountRDFStatementBuilder() {
    
}

std::vector<AccountRDFStatementPtr> AccountRDFStatementBuilder::getStatements() {
    return this->statements;
}

keto::proto::AccountInfo AccountRDFStatementBuilder::getAccountInfo() {
    return accountInfo;
}

std::string AccountRDFStatementBuilder::accountAction() {
    return this->action;
}

bool AccountRDFStatementBuilder::isNewChain() {
    return this->newChain;
}

void AccountRDFStatementBuilder::setNewChain(bool newChain) {

    // build the chain statement
    keto::asn1::RDFSubjectHelper chainRDFSubject(buildRdfUri(AccountSystemOntologyTypes::CHAIN_ONTOLOGY_CLASS,
            chainId.getHash(keto::common::StringEncoding::HEX)));
    keto::asn1::RDFPredicateHelper chainIdPredicate = buildPredicate(AccountSystemOntologyTypes::CHAIN_PREDICATES::ID,
                                                                     chainId.getHash(keto::common::StringEncoding::HEX),
                                                                     keto::asn1::Constants::RDF_TYPES::STRING,
                                                                     keto::asn1::Constants::RDF_NODE::LITERAL);
    chainRDFSubject.addPredicate(chainIdPredicate);

    this->rdfModelHelperPtr->addSubject(chainRDFSubject);

    this->newChain = newChain;

}


std::string AccountRDFStatementBuilder::buildRdfUri(const std::string uri, const std::string id) {
    std::stringstream ss;
    ss << uri << "/" << id;
    return ss.str();
}


keto::asn1::RDFPredicateHelper AccountRDFStatementBuilder::buildPredicate(const std::string& predicate,
        const std::string& value, const std::string& type, const std::string& dataType) {
    keto::asn1::RDFPredicateHelper rdfPredicate(predicate);
    keto::asn1::RDFObjectHelper objectHelper(value,dataType,type);
    rdfPredicate.addObject(objectHelper);
    return rdfPredicate;
}





}
}
