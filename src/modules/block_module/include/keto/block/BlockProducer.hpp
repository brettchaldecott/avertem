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

#include "keto/transaction/Resource.hpp"
#include "keto/crypto/KeyLoader.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/event/Event.hpp"
#include "keto/software_consensus/ConsensusMessageHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

#include "keto/block_db/SignedBlockWrapperMessageProtoHelper.hpp"


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
        void refreshTangle();

    private:
        std::mutex classMutex;
        keto::asn1::HashHelper tangleHash;
        bool existing;
        keto::asn1::HashHelper lastBlockHash;
        int numberOfAccounts;

    };
    typedef std::shared_ptr<TangleFutureStateManager> TangleFutureStateManagerPtr;

    class PendingTransactionsTangle : keto::transaction::Resource {
    public:
        PendingTransactionsTangle(const keto::asn1::HashHelper& tangleHash, bool existing = true);
        PendingTransactionsTangle(const PendingTransactionsTangle& orig) = delete;
        virtual ~PendingTransactionsTangle();


        TangleFutureStateManagerPtr getTangle();
        void addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> getTransactions();
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> takeTransactions();
        bool empty();

        // commit and rollbak
        virtual void commit();
        virtual void rollback();

    private:
        std::mutex classMutex;
        TangleFutureStateManagerPtr tangleFutureStateManagerPtr;
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> activeTransactions;
        std::deque<keto::transaction_common::TransactionProtoHelperPtr> pendingTransactions;
    };
    typedef std::shared_ptr<PendingTransactionsTangle> PendingTransactionsTanglePtr;

    class PendingTransactionManager : keto::transaction::Resource {
    public:
        PendingTransactionManager();
        PendingTransactionManager(const PendingTransactionManager& orig) = delete;
        virtual ~PendingTransactionManager();

        void addTransaction(const keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
        std::deque<PendingTransactionsTanglePtr> takeTransactions();
        bool empty();
        void clear();

        // commit and rollbak
        virtual void commit();
        virtual void rollback();

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

    class ProducerScopeLock;
    typedef std::shared_ptr<ProducerScopeLock> ProducerScopeLockPtr;

    class ProducerLock {
    public:
        friend class ProducerScopeLock;

        ProducerLock();
        ProducerLock(const ProducerLock& orig) = delete;
        virtual ~ProducerLock();

        ProducerScopeLockPtr aquireBlockLock();
        ProducerScopeLockPtr aquireTransactionLock();
    protected:
        void release(bool _transactionLock, bool _blockLock);
    public:
        std::mutex classMutex;
        std::condition_variable stateCondition;
        int transactionLock;
        int blockLock;
    };
    typedef std::shared_ptr<ProducerLock> ProducerLockPtr;

    class ProducerScopeLock {
    public:
        ProducerScopeLock(ProducerLock* reference, bool transactionLock, bool blockLock);
        ProducerScopeLock(const ProducerScopeLock& orig) = delete;
        virtual ~ProducerScopeLock();
    public:
        ProducerLock* reference;
        bool transactionLock;
        bool blockLock;
    };


    /**
     * The state enum containing the various states that the module manager can
     * be in.
     */
    enum State {
        unloaded = 1,
        inited = 2,
        block_producer_wait = 3,
        block_producer = 4,
        block_producer_complete = 5,
        terminated = 6,
        sync_blocks = 7
    };

    enum ProducerState {
        idle,
        producing,
        ending,
        complete
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
    bool isSafe();

    keto::event::Event setupNodeConsensusSession(const keto::event::Event& event);
    void addTransaction(keto::transaction_common::TransactionProtoHelperPtr& transactionProtoHelperPtr);
    bool isEnabled();
    bool isLoaded();

    keto::software_consensus::ConsensusMessageHelper getAcceptedCheck();
    keto::crypto::KeyLoaderPtr getKeyLoader();

    // setup the active tangles
    std::vector<keto::asn1::HashHelper> getActiveTangles();
    void clearActiveTangles();
    void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);
    void processProducerEnding(
            const keto::block_db::SignedBlockWrapperMessageProtoHelper& signedBlockWrapperMessageProtoHelper);
    void activateWaitingBlockProducer();

    // get transaction lock
    ProducerScopeLockPtr aquireTransactionLock();
private:
    bool enabled;
    bool loaded;
    int delay;
    State currentState;
    ProducerState producerState;
    bool safe;
    std::condition_variable stateCondition;
    std::mutex classMutex;
    PendingTransactionManagerPtr pendingTransactionManagerPtr;
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    keto::software_consensus::ConsensusMessageHelper consensusMessageHelper;
    ProducerLockPtr producerLockPtr;

    State checkState();
    void processTransactions();
    keto::block_db::SignedBlockBuilderPtr generateBlock(const BlockProducer::PendingTransactionsTanglePtr& pendingTransactionTanglePtr);

    void load();
    void sync();
    void waitingBlockProducerSync();
    void _setState(const State& state);
    State _getState();

    void setProducerState(const ProducerState& state, bool notify = false);
    void _setProducerState(const ProducerState& state, bool notify = false);
    ProducerState getProducerState();
    ProducerState _getProducerState();

    void requestNetworkState();
};


}
}


#endif /* BLOCKPRODUCER_HPP */

