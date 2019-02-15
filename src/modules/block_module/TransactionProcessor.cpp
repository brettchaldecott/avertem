/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionProcessor.cpp
 * Author: ubuntu
 * 
 * Created on April 2, 2018, 9:00 AM
 */

#include <condition_variable>
#include <iostream>

#include "keto/block/TransactionProcessor.hpp"


#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/Constants.hpp"


namespace keto {
namespace block {

static TransactionProcessorPtr singleton;

std::string TransactionProcessor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionProcessor::TransactionProcessor() {
}

TransactionProcessor::~TransactionProcessor() {
}

TransactionProcessorPtr TransactionProcessor::init() {
    return singleton = std::make_shared<TransactionProcessor>();
}

void TransactionProcessor::fin() {
    singleton.reset();
}

TransactionProcessorPtr TransactionProcessor::getInstance() {
    return singleton;
}

keto::proto::Transaction TransactionProcessor::processTransaction(keto::proto::Transaction& transaction) {
    keto::transaction_common::TransactionProtoHelper transactionProtoHelper(transaction);
    
    // get the transaction from the account store
    transactionProtoHelper.setTransaction(executeContract(
            getContractByName(transaction.active_account(),
            keto::server_common::Constants::CONTRACTS::BASE_ACCOUNT_CONTRACT),
            transactionProtoHelper).transaction());


    std::cout << "Before looping through the actions" << std::endl;
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr = 
            transactionProtoHelper.getTransactionMessageHelper();
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr = 
            transactionMessageHelperPtr->getTransactionWrapper();
    if (transactionWrapperHelperPtr->getSignedTransaction() &&
            transactionWrapperHelperPtr->getSignedTransaction()->getTransaction()) {
        std::vector<keto::transaction_common::ActionHelperPtr> actions = 
            transactionWrapperHelperPtr->getSignedTransaction()->getTransaction()->getActions();    
        for (keto::transaction_common::ActionHelperPtr action : actions) {
            std::cout << "The action is contract : " << action->getContract().getHash(keto::common::HEX) << std::endl;
            keto::asn1::AnyHelper anyHelper(*transactionMessageHelperPtr);
            transactionProtoHelper.setTransaction(executeContract(getContractByHash(transaction.active_account(),
                    action->getContract()),transactionProtoHelper,action->getModel()).transaction());
        }
        
    }

    std::cout << "Nested transactions" << std::endl;

    for (keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr :
        transactionProtoHelper.getTransactionMessageHelper()->getNestedTransactions()) {
        std::cout << "Loop through the nested transactions" << std::endl;
        keto::transaction_common::TransactionProtoHelper nestedTransaction(transactionMessageHelperPtr);
        nestedTransaction = processTransaction(nestedTransaction);
        transactionMessageHelperPtr->setTransactionWrapper(
                nestedTransaction.getTransactionMessageHelper()->getTransactionWrapper());


        keto::asn1::AnyHelper anyHelper(*transactionMessageHelperPtr);
        transactionProtoHelper.setTransaction(executeContract(getContractByName(transaction.active_account(),
                                          keto::server_common::Constants::CONTRACTS::NESTED_TRANSACTION_CONTRACT),
                                                  transactionProtoHelper,anyHelper).transaction());

    }
    std::cout << "Return the resulting transactions" << std::endl;

    return transactionProtoHelper;
}

std::string TransactionProcessor::getContractByName(const std::string& account, const std::string& name) {
    keto::proto::ContractMessage contractMessage;
    contractMessage.set_account_hash(account);
    contractMessage.set_contract_name(name);
    return getContract(contractMessage).contract();
}

std::string TransactionProcessor::getContractByHash(const std::string& account, const std::string& hash) {
    keto::proto::ContractMessage contractMessage;
    contractMessage.set_account_hash(account);
    contractMessage.set_contract_hash(hash);
    return getContract(contractMessage).contract();
}

keto::proto::ContractMessage TransactionProcessor::getContract(keto::proto::ContractMessage& contractMessage) {
    return keto::server_common::fromEvent<keto::proto::ContractMessage>(
        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::ContractMessage>(
        keto::server_common::Events::GET_CONTRACT,contractMessage)));
}

keto::proto::SandboxCommandMessage TransactionProcessor::executeContract(const std::string& contract,
        const keto::transaction_common::TransactionProtoHelper& transactionProtoHelper) {

    keto::proto::SandboxCommandMessage sandboxCommandMessage;
    sandboxCommandMessage.set_contract(contract);
    sandboxCommandMessage.set_transaction((const std::string)transactionProtoHelper);

    return keto::server_common::fromEvent<keto::proto::SandboxCommandMessage>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SandboxCommandMessage>(
                    keto::server_common::Events::EXECUTE_ACTION_MESSAGE,sandboxCommandMessage)));
}

keto::proto::SandboxCommandMessage TransactionProcessor::executeContract(const std::string& contract,
        const keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
        keto::asn1::AnyHelper model) {

    keto::proto::SandboxCommandMessage sandboxCommandMessage;
    sandboxCommandMessage.set_contract(contract);
    sandboxCommandMessage.set_transaction((const std::string)transactionProtoHelper);
    sandboxCommandMessage.set_model(model);

    return keto::server_common::fromEvent<keto::proto::SandboxCommandMessage>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SandboxCommandMessage>(
                            keto::server_common::Events::EXECUTE_ACTION_MESSAGE,sandboxCommandMessage)));
}

}
}