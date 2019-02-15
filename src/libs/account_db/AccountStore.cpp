/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountStore.cpp
 * Author: ubuntu
 * 
 * Created on March 5, 2018, 6:02 AM
 */

#include <memory>
#include <string>
#include <vector>
#include <sstream>

#include "Account.pb.h"

#include "keto/crypto/Containers.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/account_db/Constants.hpp"
#include "keto/account_db/AccountStore.hpp"
#include "keto/account_db/AccountRDFStatement.hpp"
#include "keto/account_db/AccountRDFStatementBuilder.hpp"
#include "keto/account_db/AccountSystemOntologyTypes.hpp"
#include "keto/account_db/Exception.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "include/keto/account_db/AccountRDFStatement.hpp"
#include "include/keto/account_db/AccountGraphSession.hpp"

namespace keto {
namespace account_db {

static std::shared_ptr<AccountStore> singleton;

std::string AccountStore::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountStore::AccountStore() {
    dbManagerPtr = std::shared_ptr<keto::rocks_db::DBManager>(
            new keto::rocks_db::DBManager(Constants::DB_LIST));
    accountGraphStoreManagerPtr = AccountGraphStoreManagerPtr(new AccountGraphStoreManager());
    accountResourceManagerPtr  =  AccountResourceManagerPtr(
            new AccountResourceManager(dbManagerPtr,accountGraphStoreManagerPtr));

}

AccountStore::~AccountStore() {
    accountResourceManagerPtr.reset();
    accountGraphStoreManagerPtr.reset();
    dbManagerPtr.reset();
}

std::shared_ptr<AccountStore> AccountStore::init() {
    if (!singleton) {
        singleton = std::shared_ptr<AccountStore>(new AccountStore());
    }
    return singleton;
}

void AccountStore::fin() {
    singleton.reset();
}

std::shared_ptr<AccountStore> AccountStore::getInstance() {
    return singleton;
}

bool AccountStore::getAccountInfo(const keto::asn1::HashHelper& accountHash,
            keto::proto::AccountInfo& result) {
    AccountResourcePtr resource = accountResourceManagerPtr->getResource();
    rocksdb::Transaction* accountTransaction = resource->getTransaction(Constants::ACCOUNTS_MAPPING);
    keto::rocks_db::SliceHelper accountHashHelper(keto::crypto::SecureVectorUtils().copyFromSecure(
        accountHash));
    rocksdb::ReadOptions readOptions;
    std::string value;
    if (rocksdb::Status::OK() != accountTransaction->Get(readOptions,accountHashHelper,&value)) {
        return false;
    }
    result.ParseFromString(value);
    return true;
}


void AccountStore::applyTransaction(
        const keto::asn1::HashHelper& chainId,const keto::asn1::HashHelper& blockId,
        const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr) {
    AccountResourcePtr resource = accountResourceManagerPtr->getResource();
    keto::proto::AccountInfo accountInfo;
    AccountRDFStatementBuilderPtr accountRDFStatementBuilder;
    keto::asn1::HashHelper accountHash;

    if ((transactionWrapperHelperPtr->getStatus() == Status_debit) || (transactionWrapperHelperPtr->getStatus() == Status_init)) {
        accountHash = transactionWrapperHelperPtr->getSourceAccount();
    } else if (transactionWrapperHelperPtr->getStatus() == Status_fee) {
        accountHash = transactionWrapperHelperPtr->getFeeAccount();
    } else if ((transactionWrapperHelperPtr->getStatus() == Status_credit) || (transactionWrapperHelperPtr->getStatus() == Status_complete)) {
        accountHash = transactionWrapperHelperPtr->getTargetAccount();
    }
    
    if (!getAccountInfo(accountHash,accountInfo)) {
        accountRDFStatementBuilder =  AccountRDFStatementBuilderPtr(
                new AccountRDFStatementBuilder(chainId,blockId,
                        transactionWrapperHelperPtr,false));
        createAccount(chainId, accountHash,transactionWrapperHelperPtr,accountRDFStatementBuilder,accountInfo);
    } else {
        accountRDFStatementBuilder =  AccountRDFStatementBuilderPtr(
                new AccountRDFStatementBuilder(chainId,blockId,
                        transactionWrapperHelperPtr,true));
    }
    AccountGraphSessionPtr sessionPtr = resource->getGraphSession(accountInfo.graph_name());
        
    
    for (AccountRDFStatementPtr accountRDFStatement : accountRDFStatementBuilder->getStatements()) {
        keto::asn1::RDFModelHelperPtr rdfModel = accountRDFStatement->getModel();
        for (keto::asn1::RDFSubjectHelperPtr rdfSubject : rdfModel->getSubjects()) {
            if (accountRDFStatement->getOperation() == PERSIST) {
                std::cout << "This is an attempt to persist the subject" << std::endl;
                sessionPtr->persist(rdfSubject);
            } else {
                sessionPtr->remove(rdfSubject);
            }
        }
    }
}

void AccountStore::sparqlQuery(
        const keto::proto::AccountInfo& accountInfo,
        keto::proto::SparqlQuery& sparlQuery) {
    AccountResourcePtr resource = accountResourceManagerPtr->getResource();
    AccountGraphSessionPtr sessionPtr = resource->getGraphSession(accountInfo.graph_name());
    sparlQuery.set_result(sessionPtr->query(sparlQuery.query()));
}

void AccountStore::getContract(
        const keto::proto::AccountInfo& accountInfo,
        keto::proto::ContractMessage& contractMessage) {
    AccountResourcePtr resource = accountResourceManagerPtr->getResource();
    AccountGraphSessionPtr sessionPtr = resource->getGraphSession(accountInfo.graph_name());
    std::stringstream ss;
    keto::asn1::HashHelper accountHash(contractMessage.account_hash());
    if (!contractMessage.contract_name().empty()) {

        ss << "SELECT ?code WHERE { " <<
           "?contract <http://keto-coin.io/schema/rdf/1.0/keto/Contract#name> '" << contractMessage.contract_name() << "'^^<http://www.w3.org/2001/XMLSchema#string> . " <<
           "?contract <http://keto-coin.io/schema/rdf/1.0/keto/Contract#accountHash> '" << accountHash.getHash(keto::common::StringEncoding::HEX) << "'^^<http://www.w3.org/2001/XMLSchema#string> . " <<
           "?contractVersion <http://keto-coin.io/schema/rdf/1.0/keto/ContractVersion#contract> ?contract . " <<
           "?contractVersion <http://keto-coin.io/schema/rdf/1.0/keto/ContractVersion#dateTime> ?dateTime . " <<
           "?contractVersion <http://keto-coin.io/schema/rdf/1.0/keto/ContractVersion#value> ?code . } " <<
           "ORDER BY DESC (?dateTime) LIMIT 1";

    } else {
        keto::asn1::HashHelper contractHash(contractMessage.contract_hash());
        ss << "SELECT ?code WHERE { " <<
            "?contract <http://keto-coin.io/schema/rdf/1.0/keto/Contract#hash> '" << contractHash.getHash(keto::common::StringEncoding::HEX) << "'^^<http://www.w3.org/2001/XMLSchema#string> . " <<
            "?contract <http://keto-coin.io/schema/rdf/1.0/keto/Contract#accountHash> '" << accountHash.getHash(keto::common::StringEncoding::HEX) << "'^^<http://www.w3.org/2001/XMLSchema#string> . " <<
            "?contractVersion <http://keto-coin.io/schema/rdf/1.0/keto/ContractVersion#contract> ?contract . " <<
            "?contractVersion <http://keto-coin.io/schema/rdf/1.0/keto/ContractVersion#dateTime> ?dateTime . " <<
            "?contractVersion <http://keto-coin.io/schema/rdf/1.0/keto/ContractVersion#value> ?code . } " <<
            "ORDER BY DESC (?dateTime) LIMIT 1";
    }
    ResultVectorMap result = sessionPtr->executeQuery(ss.str());
    if (result.size() == 1) {
        contractMessage.set_contract(result[0]["code"]);
    } else {
        BOOST_THROW_EXCEPTION(keto::account_db::UnknownContractException());
    }
    
}


void AccountStore::createAccount(
            const keto::asn1::HashHelper& chainId,
            const keto::asn1::HashHelper& accountHash,
            const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr,
            AccountRDFStatementBuilderPtr accountRDFStatementBuilder,
            keto::proto::AccountInfo& accountInfo) {
    if (accountRDFStatementBuilder->accountAction().compare(
                AccountSystemOntologyTypes::ACCOUNT_CREATE_OBJECT_STATUS)) {
        std::stringstream ss;
        ss << "The account does not exist [" << 
                accountHash.getHash(keto::common::HEX) << "]";
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidAccountOperationException(
                ss.str()));
    }
    accountInfo = accountRDFStatementBuilder->getAccountInfo();

    if (accountInfo.account_type().compare(AccountSystemOntologyTypes::ACCOUNT_TYPE::MASTER) ==0) {
        // We assume that the top most master account will not have a parent
        // we thus assume this will be during the genesis process and we set it to the base graph
        // for all future masters we set it to the hex hash value for the master account hash.
        if (accountInfo.parent_account_hash().empty()) {
            accountInfo.set_graph_name(Constants::BASE_GRAPH);
        } else {
            accountInfo.set_graph_name(chainId.getHash(keto::common::HEX));
        }
        if (!this->accountGraphStoreManagerPtr->checkForDb(accountInfo.graph_name())) {
            this->accountGraphStoreManagerPtr->createStore(accountInfo.graph_name());
            accountRDFStatementBuilder->setNewChain(true);
        }
    } else {
        keto::asn1::HashHelper parentAccountHash(
                    accountInfo.parent_account_hash());
        keto::proto::AccountInfo parentAccountInfo;
        if (!getAccountInfo(parentAccountHash,parentAccountInfo)) {
            std::stringstream ss;
            ss << "The parent account [" << parentAccountHash.getHash(keto::common::HEX) << 
                    "] was not found for the account [" << 
                    accountHash.getHash(keto::common::HEX) << "]["
                    << accountInfo.account_type() << "]";
            BOOST_THROW_EXCEPTION(keto::account_db::InvalidParentAccountException(
                    ss.str()));
        }
        accountInfo.set_graph_name(parentAccountInfo.graph_name());
    }
    setAccountInfo(accountHash,accountInfo);
}

void AccountStore::setAccountInfo(const keto::asn1::HashHelper& accountHash,
            keto::proto::AccountInfo& accountInfo) {
    AccountResourcePtr resource = accountResourceManagerPtr->getResource();
    rocksdb::Transaction* accountTransaction = resource->getTransaction(Constants::ACCOUNTS_MAPPING);
    keto::rocks_db::SliceHelper accountHashHelper(keto::crypto::SecureVectorUtils().copyFromSecure(
        accountHash));
    std::string pbValue;
    accountInfo.SerializeToString(&pbValue);
    keto::rocks_db::SliceHelper valueHelper(pbValue);
    accountTransaction->Put(accountHashHelper,valueHelper);
}


void AccountStore::getNodeAccountRouting(const keto::asn1::HashHelper& accountHash,
            keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    AccountResourcePtr resource = accountResourceManagerPtr->getResource();
    rocksdb::Transaction* accountTransaction = resource->getTransaction(Constants::ACCOUNTS_MAPPING);
    rocksdb::ReadOptions readOptions;
    rocksdb::Iterator* iterator = accountTransaction->GetIterator(readOptions);
    iterator->SeekToFirst();
    std::vector<keto::proto::AccountInfo> accountInfoVector;
    while (iterator->Valid()) {
        keto::rocks_db::SliceHelper valueHelper(iterator->value());
        keto::proto::AccountInfo accountInfo;
        accountInfo.ParseFromString((const std::string)valueHelper);
        accountInfoVector.push_back(accountInfo);
        iterator->Next();
    }
    rpcPeerHelper.setAccountHash(accountHash);
    keto::router_utils::PushAccountHelper pushAccountHelper;
    pushAccountHelper.setAccountHash(accountHash);
    buildNodeAccountRouting(accountHash,accountInfoVector,pushAccountHelper);
    rpcPeerHelper.setPushAccount(pushAccountHelper);
}


void AccountStore::buildNodeAccountRouting(const std::string& accountHash,
            const std::vector<keto::proto::AccountInfo>& accountInfoVector,
            keto::router_utils::PushAccountHelper& pushAccountHelper) {
    for (const keto::proto::AccountInfo& accountInfo : accountInfoVector) {
        if (accountHash == accountInfo.parent_account_hash()) {
            keto::router_utils::PushAccountHelper childPushAccountHelper;
            childPushAccountHelper.setAccountHash(accountInfo.account_hash());
            childPushAccountHelper.setManagementAccountHash(accountHash);
            buildNodeAccountRouting(accountInfo.account_hash(),
                    accountInfoVector,childPushAccountHelper);
            pushAccountHelper.addChildAccount(childPushAccountHelper);
        }
    }
    
}


}
}
