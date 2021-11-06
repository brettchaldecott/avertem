/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountService.cpp
 * Author: ubuntu
 * 
 * Created on March 6, 2018, 1:41 PM
 */

#include <string>
#include <iostream>

#include "Account.pb.h"
#include "BlockChain.pb.h"
#include "Sparql.pb.h"
#include "Contract.pb.h"


#include "keto/asn1/HashHelper.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/account/AccountService.hpp"
#include "keto/account_db/AccountStore.hpp"
#include "keto/account_db/Constants.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"
#include "keto/transaction_common/AccountTransactionInfoProtoHelper.hpp"
#include "keto/account_db/AccountGraphDirtySessionManager.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace account {

static std::shared_ptr<AccountService> singleton;

std::string AccountService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountService::AccountService() {
}

AccountService::~AccountService() {
}

// account service management methods
std::shared_ptr<AccountService> AccountService::init() {
    if (!singleton) {
        singleton = std::shared_ptr<AccountService>(new AccountService());
    }
    return singleton;
}

void AccountService::fin() {
    singleton.reset();
}

std::shared_ptr<AccountService> AccountService::getInstance() {
    return singleton;
}

keto::event::Event AccountService::applyDirtyTransaction(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::applyDirtyTransaction] Apply the trasaction dirty";
    keto::proto::AccountTransactionInfo transaction =
            keto::server_common::fromEvent<keto::proto::AccountTransactionInfo>(event);
    keto::transaction_common::AccountTransactionInfoProtoHelper accountTransactionInfoProtoHelper(transaction);
    keto::account_db::AccountStore::getInstance()->applyDirtyTransaction(
            accountTransactionInfoProtoHelper.getBlockChainId(),
            accountTransactionInfoProtoHelper.getTransaction());

    //KETO_LOG_INFO << "[AccountService::applyDirtyTransaction] Applied the transaction dirty";
    return keto::server_common::toEvent<keto::proto::AccountTransactionInfo>(transaction);
}

keto::event::Event AccountService::applyTransaction(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::applyTransaction] Apply the transaction";
    keto::proto::AccountTransactionInfo transaction =
            keto::server_common::fromEvent<keto::proto::AccountTransactionInfo>(event);
    keto::transaction_common::AccountTransactionInfoProtoHelper accountTransactionInfoProtoHelper(transaction);
    keto::account_db::AccountStore::getInstance()->applyTransaction(
        accountTransactionInfoProtoHelper.getBlockChainId(),accountTransactionInfoProtoHelper.getBlockId(),
        accountTransactionInfoProtoHelper.getTransaction());
    
    //KETO_LOG_INFO << "[AccountService::applyTransaction] Applied the transaction";
    return keto::server_common::toEvent<keto::proto::AccountTransactionInfo>(transaction);
}
    

// account methods
keto::event::Event AccountService::checkAccount(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::checkAccount] Check the account";
    keto::proto::CheckForAccount  checkForAccount = 
            keto::server_common::fromEvent<keto::proto::CheckForAccount>(event);
    
    keto::proto::AccountInfo accountInfo;
    keto::asn1::HashHelper accountHashHelper(checkForAccount.account_hash());
    if (keto::account_db::AccountStore::getInstance()->getAccountInfo(accountHashHelper,
            accountInfo)) {
        checkForAccount.set_found(true);
    } else {
        checkForAccount.set_found(false);
    }
    //KETO_LOG_INFO << "[AccountService::checkAccount] Checked the account";
    return keto::server_common::toEvent<keto::proto::CheckForAccount>(checkForAccount);
}


keto::event::Event AccountService::sparqlQuery(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::sparqlQuery] Perform a query";
    keto::proto::SparqlQuery  sparqlQuery = 
            keto::server_common::fromEvent<keto::proto::SparqlQuery>(event);
    
    keto::proto::AccountInfo accountInfo;
    keto::asn1::HashHelper accountHashHelper(sparqlQuery.account_hash());
    if (keto::account_db::AccountStore::getInstance()->getAccountInfo(accountHashHelper,
            accountInfo)) {
        keto::account_db::AccountStore::getInstance()->sparqlQuery(accountInfo,sparqlQuery);
    }
    //KETO_LOG_INFO << "[AccountService::sparqlQuery] After performing the query";
    return keto::server_common::toEvent<keto::proto::SparqlQuery>(sparqlQuery);
}

keto::event::Event AccountService::sparqlQueryWithResultSet(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::sparqlQueryWithResultSet] Perform the sparql query";
    keto::proto::SparqlResultSetQuery  sparqlQuery =
            keto::server_common::fromEvent<keto::proto::SparqlResultSetQuery>(event);

    keto::proto::AccountInfo accountInfo;
    keto::asn1::HashHelper accountHashHelper(sparqlQuery.account_hash());
    keto::proto::SparqlResultSet sparqlResultSet;
    if (keto::account_db::AccountStore::getInstance()->getAccountInfo(accountHashHelper,
                                                                      accountInfo)) {
        sparqlResultSet =
                keto::account_db::AccountStore::getInstance()->sparqlQueryWithResultSet(accountInfo,sparqlQuery);
    } else {
        KETO_LOG_INFO << "[sparqlQueryWithResultSet]The account [" << accountHashHelper.getHash(keto::common::StringEncoding::HEX) << "]";
    }

    //KETO_LOG_INFO << "[AccountService::sparqlQueryWithResultSet] Performed the sparql query";
    return keto::server_common::toEvent<keto::proto::SparqlResultSet>(sparqlResultSet);
}

keto::event::Event AccountService::dirtySparqlQueryWithResultSet(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::dirtySparqlQueryWithResultSet] Perform the dirty sparql query";
    keto::proto::SparqlResultSetQuery  sparqlQuery =
            keto::server_common::fromEvent<keto::proto::SparqlResultSetQuery>(event);

    keto::proto::AccountInfo accountInfo;
    keto::asn1::HashHelper accountHashHelper(sparqlQuery.account_hash());
    keto::proto::SparqlResultSet sparqlResultSet;
    if (keto::account_db::AccountStore::getInstance()->getAccountInfo(accountHashHelper,
                                                                      accountInfo)) {
        sparqlResultSet = keto::account_db::AccountStore::getInstance()->dirtySparqlQueryWithResultSet(accountInfo,sparqlQuery);

    }
    //KETO_LOG_INFO << "[AccountService::dirtySparqlQueryWithResultSet] Performed the dirty sparql query";
    return keto::server_common::toEvent<keto::proto::SparqlResultSet>(sparqlResultSet);
}

keto::event::Event AccountService::getContract(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::getContract] Get the contract";
    keto::proto::ContractMessage  contractMessage =
            keto::server_common::fromEvent<keto::proto::ContractMessage>(event);
    keto::proto::AccountInfo accountInfo;
    keto::asn1::HashHelper accountHashHelper(contractMessage.account_hash());
    if (keto::account_db::AccountStore::getInstance()->getAccountInfo(accountHashHelper,
            accountInfo)) {

        try {
            keto::account_db::AccountStore::getInstance()->getContract(accountInfo, contractMessage);
            KETO_LOG_INFO << "[AccountService::getContract] Return the contract attached to the account";
            return keto::server_common::toEvent<keto::proto::ContractMessage>(contractMessage);
        } catch (...) {

        }
    }

    // fall back to the master hash
    accountHashHelper.setHash(keto::account_db::Constants::BASE_MASTER_ACCOUNT_HASH,keto::common::HEX);
    accountInfo.set_account_hash(accountHashHelper);
    accountInfo.set_graph_name(keto::account_db::Constants::BASE_GRAPH);
    keto::account_db::AccountStore::getInstance()->getContract(accountInfo,contractMessage);

    //KETO_LOG_INFO << "[AccountService::getContract] Return the global contract";
    return keto::server_common::toEvent<keto::proto::ContractMessage>(contractMessage);
}

keto::event::Event AccountService::clearDirty(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[AccountService::clearDirty] Clear the dirty session";
    keto::account_db::AccountGraphDirtySessionManager::getInstance()->clearSessions();
    //KETO_LOG_INFO << "[AccountService::clearDirty] Cleared the dirty session";
    return event;
}

}
}