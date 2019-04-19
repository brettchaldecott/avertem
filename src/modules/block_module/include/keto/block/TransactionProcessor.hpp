/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionProcessor.hpp
 * Author: ubuntu
 *
 * Created on April 2, 2018, 9:00 AM
 */

#ifndef TRANSACTIONPROCESSOR_HPP
#define TRANSACTIONPROCESSOR_HPP

#include <memory>
#include <string>

#include "Sandbox.pb.h"
#include "Contract.pb.h"
#include "BlockChain.pb.h"

#include "keto/common/MetaInfo.hpp"

#include "keto/transaction_common/ActionHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"


namespace keto {
namespace block {

class TransactionProcessor;
typedef std::shared_ptr<TransactionProcessor> TransactionProcessorPtr;

class TransactionProcessor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    class TransactionTracker {
    public:
        TransactionTracker(long availableTime, long elapsedTime, float feeRatio);
        TransactionTracker(const TransactionTracker& orig) = default;
        virtual ~TransactionTracker();

        long getAvailableTime();
        long getElapsedTime();
        long incrementElapsedTime(int increase);
        float getFeeRatio();
    private:
        long availableTime;
        long elapsedTime;
        float feeRatio;
    };
    typedef std::shared_ptr<TransactionTracker> TransactionTrackerPtr;

    TransactionProcessor();
    TransactionProcessor(const TransactionProcessor& orig) = delete;
    virtual ~TransactionProcessor();
    
    static TransactionProcessorPtr init();
    static void fin();
    static TransactionProcessorPtr getInstance();
    
    keto::proto::Transaction processTransaction(keto::proto::Transaction& transaction);
    
private:

    keto::proto::ContractMessage getContractByName(const keto::asn1::HashHelper& account, const std::string& name);
    keto::proto::ContractMessage getContractByHash(const keto::asn1::HashHelper& account, const std::string& hash);
    keto::proto::ContractMessage getContract(keto::proto::ContractMessage& contractMessage);

    keto::proto::SandboxCommandMessage executeContract(const keto::proto::ContractMessage& contract,
            const keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
            TransactionTracker& transactionTracker);
    keto::proto::SandboxCommandMessage executeContract(const keto::proto::ContractMessage& contract,
            const keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
            const keto::asn1::AnyHelper& model, TransactionTracker& transactionTracker);

    keto::transaction_common::TransactionProtoHelper processTransaction(
            keto::transaction_common::TransactionProtoHelper& transactionProtoHelper,
            bool master, TransactionTracker& transactionTracker);
};


}
}

#endif /* TRANSACTIONPROCESSOR_HPP */

