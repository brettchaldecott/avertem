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

#include <math.h>
#include <condition_variable>
#include <iostream>
#include <keto/block/Constants.hpp>

#include "keto/environment/Units.hpp"
#include "keto/block/TransactionProcessor.hpp"
#include "keto/block/Exception.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"

#include "keto/block/NetworkFeeManager.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/Constants.hpp"

#include "keto/transaction_common/AccountTransactionInfoProtoHelper.hpp"

namespace keto {
namespace block {

static TransactionProcessorPtr singleton;

std::string TransactionProcessor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionProcessor::TransactionTracker::TransactionTracker(long availableTime, long elapsedTime, float feeRatio) {
    this->availableTime = availableTime;
    this->elapsedTime = elapsedTime;
    this->feeRatio = feeRatio;
}

TransactionProcessor::TransactionTracker::~TransactionTracker() {

}

long TransactionProcessor::TransactionTracker::getAvailableTime() {
    return this->availableTime;
}

long TransactionProcessor::TransactionTracker::getElapsedTime() {
    return this->elapsedTime;
}

long TransactionProcessor::TransactionTracker::incrementElapsedTime(int increase) {
    return this->elapsedTime += increase;
}

float TransactionProcessor::TransactionTracker::getFeeRatio() {
    return this->feeRatio;
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

    // calculate the available time based
    TransactionTrackerPtr transactionTrackerPtr;
    keto::transaction_common::FeeInfoMsgProtoHelperPtr feeInfoMsgProtoHelperPtr = NetworkFeeManager::getInstance()->getFeeInfo();
    if (transactionProtoHelper.getTransactionMessageHelper()->getAvailableTime() == 0) {
        long amount = transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getSignedTransaction()->getTransaction()->getValue();
        if (amount > feeInfoMsgProtoHelperPtr->getMaxFee()) {
            transactionProtoHelper.getTransactionMessageHelper()->setAvailableTime(Constants::MAX_RUN_TIME);
        } else {
            transactionProtoHelper.getTransactionMessageHelper()->setAvailableTime(
                    feeInfoMsgProtoHelperPtr->getFeeRatio() * amount);
        }
    }
    // setup a transaction tacker using milly seconds
    TransactionTracker transactionTracker((transactionProtoHelper.getTransactionMessageHelper()->getAvailableTime() -
                                          transactionProtoHelper.getTransactionMessageHelper()->getElapsedTime()) * keto::environment::Units::TIME::MILLISECONDS,
                                          0,
                                          feeInfoMsgProtoHelperPtr->getFeeRatio());

    transactionProtoHelper = processTransaction(transactionProtoHelper,true,transactionTracker);

    //std::cout << "Return the resulting transactions" << std::endl;

    return transactionProtoHelper;
}

keto::proto::ContractMessage TransactionProcessor::getContractByName(const keto::asn1::HashHelper& account, const std::string& name) {
    keto::proto::ContractMessage contractMessage;
    contractMessage.set_account_hash(account);
    contractMessage.set_contract_name(name);
    return getContract(contractMessage);
}

keto::proto::ContractMessage TransactionProcessor::getContractByHash(const keto::asn1::HashHelper& account, const std::string& hash) {
    keto::proto::ContractMessage contractMessage;
    contractMessage.set_account_hash(account);
    contractMessage.set_contract_hash(hash);
    return getContract(contractMessage);
}

keto::proto::ContractMessage TransactionProcessor::getContract(keto::proto::ContractMessage& contractMessage) {
    return keto::server_common::fromEvent<keto::proto::ContractMessage>(
        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::ContractMessage>(
        keto::server_common::Events::GET_CONTRACT,contractMessage)));
}

keto::proto::SandboxCommandMessage TransactionProcessor::executeContract(const keto::proto::ContractMessage& contract,
        const keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
        TransactionTracker& transactionTracker) {

    keto::proto::SandboxCommandMessage sandboxCommandMessage;
    sandboxCommandMessage.set_contract(contract.contract());
    sandboxCommandMessage.set_contract_hash(contract.contract_hash());
    sandboxCommandMessage.set_contract_name(contract.contract_name());
    sandboxCommandMessage.set_transaction((const std::string)transactionProtoHelper);
    sandboxCommandMessage.set_available_time(transactionTracker.getAvailableTime());
    sandboxCommandMessage.set_elapsed_time(transactionTracker.getElapsedTime());
    sandboxCommandMessage.set_fee_ratio(transactionTracker.getFeeRatio());

    sandboxCommandMessage = keto::server_common::fromEvent<keto::proto::SandboxCommandMessage>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SandboxCommandMessage>(
                    keto::server_common::Events::EXECUTE_ACTION_MESSAGE,sandboxCommandMessage)));
    transactionTracker.incrementElapsedTime(sandboxCommandMessage.elapsed_time() - transactionTracker.getElapsedTime());
    return sandboxCommandMessage;
}

keto::proto::SandboxCommandMessage TransactionProcessor::executeContract(const keto::proto::ContractMessage& contract,
        const keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
        const keto::asn1::AnyHelper& model,
        TransactionTracker& transactionTracker) {

    keto::proto::SandboxCommandMessage sandboxCommandMessage;
    sandboxCommandMessage.set_contract(contract.contract());
    sandboxCommandMessage.set_contract_hash(contract.contract_hash());
    sandboxCommandMessage.set_contract_name(contract.contract_name());
    sandboxCommandMessage.set_transaction((const std::string)transactionProtoHelper);
    sandboxCommandMessage.set_model(model);
    sandboxCommandMessage.set_available_time(transactionTracker.getAvailableTime());
    sandboxCommandMessage.set_elapsed_time(transactionTracker.getElapsedTime());
    sandboxCommandMessage.set_fee_ratio(transactionTracker.getFeeRatio());

    sandboxCommandMessage = keto::server_common::fromEvent<keto::proto::SandboxCommandMessage>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SandboxCommandMessage>(
                            keto::server_common::Events::EXECUTE_ACTION_MESSAGE,sandboxCommandMessage)));
    transactionTracker.incrementElapsedTime(sandboxCommandMessage.elapsed_time() - transactionTracker.getElapsedTime());
    return sandboxCommandMessage;
}

keto::transaction_common::TransactionProtoHelper TransactionProcessor::processTransaction(
        keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
        bool master, TransactionTracker& transactionTracker) {



    // get the transaction from the account store
    //std::cout << "The accout" << std::endl;
    keto::asn1::HashHelper currentAccount = transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getCurrentAccount();
    //std::cout << "The status : " << transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getStatus() << std::endl;
    if ((transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getStatus() == Status_init) ||
        (transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getStatus() == Status_debit)) {
        transactionProtoHelper.setTransaction(executeContract(
                getContractByName(currentAccount, keto::server_common::Constants::CONTRACTS::BASE_ACCOUNT_CONTRACT),
                transactionProtoHelper,transactionTracker).transaction());
    }


    //std::cout << "Before looping through the actions" << std::endl;
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            transactionProtoHelper.getTransactionMessageHelper();
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr =
            transactionMessageHelperPtr->getTransactionWrapper();
    if (transactionWrapperHelperPtr->getSignedTransaction() &&
        transactionWrapperHelperPtr->getSignedTransaction()->getTransaction()) {
        std::vector<keto::transaction_common::ActionHelperPtr> actions =
                transactionWrapperHelperPtr->getSignedTransaction()->getTransaction()->getActions();
        for (keto::transaction_common::ActionHelperPtr action : actions) {
            //std::cout << "The action is contract : " << action->getContract().getHash(keto::common::HEX) << std::endl;
            keto::asn1::AnyHelper anyHelper(*transactionMessageHelperPtr);
            if (action->getContract().empty()) {
                transactionProtoHelper.setTransaction(executeContract(
                        getContractByHash(currentAccount,action->getContract()),transactionProtoHelper,action->getModel(),transactionTracker)
                        .transaction());
            } else {
                transactionProtoHelper.setTransaction(executeContract(getContractByName(currentAccount,
                        action->getContractName()),transactionProtoHelper,action->getModel(),transactionTracker).transaction());
            }
        }

    }

    //std::cout << "Nested transactions" << std::endl;

    for (keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr :
            transactionProtoHelper.getTransactionMessageHelper()->getNestedTransactions()) {
        //std::cout << "Loop through the nested transactions" << std::endl;
        transactionMessageHelperPtr->getTransactionWrapper()->setStatus(
                transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getStatus());
        keto::transaction_common::TransactionProtoHelper nestedTransaction(transactionMessageHelperPtr);
        //std::cout << "Current the status : " << nestedTransaction.getTransactionMessageHelper()->getTransactionWrapper()->getStatus() << std::endl;
        nestedTransaction = processTransaction(nestedTransaction,false,transactionTracker);
        //std::cout << "Copy the transaction" << std::endl;
        transactionMessageHelperPtr->setTransactionWrapper(
                nestedTransaction.getTransactionMessageHelper()->getTransactionWrapper());

        //std::cout << "Copy the any helper" << std::endl;
        keto::asn1::AnyHelper anyHelper(*transactionMessageHelperPtr);
        //std::cout << "The process the transaction" << std::endl;
        transactionProtoHelper.setTransaction(executeContract(getContractByName(currentAccount,
                keto::server_common::Constants::CONTRACTS::NESTED_TRANSACTION_CONTRACT),
                        transactionProtoHelper,anyHelper,transactionTracker).transaction());
        //std::cout << "After the transaction" << std::endl;

    }

    if (master) {
        // set the elapsed time on the transaction
        transactionProtoHelper.getTransactionMessageHelper()->setElapsedTime(
                transactionProtoHelper.getTransactionMessageHelper()->getElapsedTime() +
                        round(transactionTracker.getElapsedTime() / keto::environment::Units::TIME::MILLISECONDS));

        // set the transaction
        transactionProtoHelper.setTransaction(executeContract(
                getContractByName(currentAccount,
                                  keto::server_common::Constants::CONTRACTS::FEE_PAYMENT_CONTRACT),
                transactionProtoHelper,transactionTracker).transaction());

    }

    if (transactionProtoHelper.getTransactionMessageHelper()->getTransactionWrapper()->getStatus() == Status_credit) {
        transactionProtoHelper.setTransaction(executeContract(
                getContractByName(currentAccount, keto::server_common::Constants::CONTRACTS::BASE_ACCOUNT_CONTRACT),
                transactionProtoHelper,transactionTracker).transaction());
    }


    return transactionProtoHelper;
}

}
}