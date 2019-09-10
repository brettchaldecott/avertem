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

    class TangleFutureStateManager {
    public:
        TangleFutureStateManager(const keto::asn1::HashHelper& tangleHash, bool existing = true);
        TangleFutureStateManager(const TangleFutureStateManager& orig) = delete;
        ~TangleFutureStateManager();

        bool isExisting();
        keto::asn1::HashHelper getTangleHash();
        keto::asn1::HashHelper getLastBlockHash();
        int getNumerOfAccounts();
        int incrementNumberOfAccounts();

    private:
        keto::asn1::HashHelper tangleHash;
        bool existing;
        keto::asn1::HashHelper lastBlockHash;
        int numberOfAccounts;

    };
    typedef std::shared_ptr<TangleFutureStateManager> TangleFutureStateManagerPtr;

    class PendingTransactionsTangle {
    public:
        PendingTransactionsTangle(const keto::asn1::HashHelper& tangleHash, bool existing = true);
        PendingTransactionsTangle(const PendingTransactionsTangle& orig) = delete;
        virtual ~PendingTransactionsTangle();


        TangleFutureStateManagerPtr getTangle();
        void addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> getTransactions();
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> takeTransactions();
        bool empty();

    private:
        TangleFutureStateManagerPtr tangleFutureStateManagerPtr;
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> transactions;
    };
    typedef std::shared_ptr<PendingTransactionsTangle> PendingTransactionsTanglePtr;

    class PendingTransactionManager {
    public:
        PendingTransactionManager();
        PendingTransactionManager(const PendingTransactionManager& orig) = delete;
        virtual ~PendingTransactionManager();

        void addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
        std::deque<PendingTransactionsTanglePtr> takeTransactions();
        bool empty();
        void clear();

    private:
        std::mutex classMutex;
        bool _empty;
        std::map<std::vector<uint8_t>,PendingTransactionsTanglePtr> tangleTransactions;
        std::deque<PendingTransactionsTanglePtr> pendingTransactions;
        PendingTransactionsTanglePtr growTanglePtr;

        PendingTransactionsTanglePtr getPendingTransactionTangle(const keto::asn1::HashHelper& tangleHash, bool existing = true);
        PendingTransactionsTanglePtr getGrowingPendingTransactionTangle();

    };
    typedef std::shared_ptr<PendingTransactionManager> PendingTransactionManagerPtr;


    /**
     * The state enum containing the various states that the module manager can
     * be in.
     */
    enum State {
        unloaded,
        inited,
        block_producer,
        block_producer_complete,
        terminated,
        sync_blocks
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
    void loadState(const State& state = State::sync_blocks);
    State getState();

    keto::event::Event setupNodeConsensusSession(const keto::event::Event& event);
    void addTransaction(keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
    bool isEnabled();
    bool isLoaded();

    keto::software_consensus::ConsensusMessageHelper getAcceptedCheck();
    keto::crypto::KeyLoaderPtr getKeyLoader();

    // setup the active tangles
    std::vector<keto::asn1::HashHelper> getActiveTangles();
    void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);

private:
    bool enabled;
    bool loaded;
    int delay;
    State currentState;
    std::condition_variable stateCondition;
    std::mutex classMutex;
    PendingTransactionManagerPtr pendingTransactionManagerPtr;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    keto::software_consensus::ConsensusMessageHelper consensusMessageHelper;


    State checkState();
    void processTransactions();
    void generateBlock(const BlockProducer::PendingTransactionsTanglePtr& pendingTransactionTanglePtr);

    void load();
    void sync();
    void _setState(const State& state);
    State _getState();
};


}
}


#endif /* BLOCKPRODUCER_HPP */

