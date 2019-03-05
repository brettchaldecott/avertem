/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockProducer.hpp
 * Author: ubuntu
 *
 * Created on April 2, 2018, 10:38 AM
 */

#ifndef BLOCKPRODUCER_HPP
#define BLOCKPRODUCER_HPP

#include <string>
#include <thread>
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>


#include "BlockChain.pb.h"

#include "keto/crypto/KeyLoader.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/event/Event.hpp"
#include "keto/software_consensus/ConsensusMessageHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"


namespace keto {
namespace block {
    
class BlockProducer;
typedef std::shared_ptr<BlockProducer> BlockProducerPtr;

class BlockProducer {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    /**
     * The state enum containing the various states that the module manager can
     * be in.
     */
    enum State {
        inited,
        block_producer,
        block_producer_complete,
        terminated
    };
    
    BlockProducer();
    BlockProducer(const BlockProducer& orig) = delete;
    virtual ~BlockProducer();
    
    static BlockProducerPtr init();
    static void fin();
    static BlockProducerPtr getInstance();
    
    void run();
    void terminate();
    void setState(const State& state);
    State getState();

    keto::event::Event setupNodeConsensusSession(const keto::event::Event& event);
    
    void addTransaction(keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
    
    bool isEnabled();
    
private:
    bool enabled;
    State currentState;
    std::condition_variable stateCondition;
    std::mutex classMutex;
    std::deque<keto::proto::Transaction> transactions;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    keto::software_consensus::ConsensusMessageHelper consensusMessageHelper;


    State checkState();
    std::deque<keto::proto::Transaction> getTransactions();
    void generateBlock(std::deque<keto::proto::Transaction> transactions);
};


}
}


#endif /* BLOCKPRODUCER_HPP */

