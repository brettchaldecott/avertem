//
// Created by Brett Chaldecott on 2019/02/06.
//

#ifndef KETO_BLOCKCHAIN_HPP
#define KETO_BLOCKCHAIN_HPP

#include <string>
#include <memory>

#include "SignedBlock.h"
#include "BlockChain.pb.h"

#include "keto/crypto/KeyLoader.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockChainMeta.hpp"
#include "keto/block_db/SignedBlockBuilder.hpp"
#include "keto/block_db/BlockChainCallback.hpp"
#include "keto/block_db/SignedBlockWrapperProtoHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {

class BlockChain;
typedef std::shared_ptr<BlockChain> BlockChainPtr;

class BlockChain {
public:

    class BlockChainCache;
    typedef std::shared_ptr<BlockChainCache> BlockChainCachePtr;
    class BlockChainCache {
    public:
        BlockChainCache();
        BlockChainCache(const BlockChainCache& orig) = delete;
        virtual ~BlockChainCache();

        static BlockChainCachePtr createInstance();
        static BlockChainCachePtr getInstance();
        static void clear();
        static void fin();

        BlockChainPtr getBlockChain(const keto::asn1::HashHelper& parentHash);
        void addBlockChain(const keto::asn1::HashHelper& transactionHash, const BlockChainPtr& blockChainPtr);

    private:
        std::map<std::string,BlockChainPtr> transactionIdBlockChainMap;

        void clearCache();
    };

    class TangleManagerInterface {
    public:
        virtual void addTangle(const keto::asn1::HashHelper& tangle) = 0;
        virtual std::vector<keto::asn1::HashHelper> getActiveTangles() = 0;
        virtual void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) = 0;
        //virtual keto::asn1::HashHelper getTangleHash() = 0;
        virtual keto::asn1::HashHelper getParentHash() = 0;
        //virtual keto::asn1::HashHelper selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id) = 0;
        virtual keto::asn1::HashHelper selectParentHash() = 0;
        virtual void setCurrentTangle(const keto::asn1::HashHelper& tangle) = 0;
    };
    typedef std::shared_ptr<TangleManagerInterface> TangleManagerInterfacePtr;

    class MasterTangleManager : public TangleManagerInterface {
    public:
        MasterTangleManager(const BlockChainMetaPtr& blockChainMetaPtr);
        MasterTangleManager(const MasterTangleManager& orig) = delete;
        virtual ~MasterTangleManager();

        virtual void addTangle(const keto::asn1::HashHelper& tangle);
        virtual std::vector<keto::asn1::HashHelper> getActiveTangles();
        virtual void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);
        //virtual keto::asn1::HashHelper getTangleHash();
        virtual keto::asn1::HashHelper getParentHash();
        //virtual keto::asn1::HashHelper selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id);
        virtual keto::asn1::HashHelper selectParentHash();
        virtual void setCurrentTangle(const keto::asn1::HashHelper& tangle);

    private:
        BlockChainMetaPtr blockChainMetaPtr;
        std::map<std::vector<uint8_t>,BlockChainTangleMetaPtr> activeTangles;
        BlockChainTangleMetaPtr currentTangle;


    };

    class NestedTangleManager : public TangleManagerInterface {
    public:
        NestedTangleManager(const BlockChainMetaPtr& blockChainMetaPtr);
        NestedTangleManager(const NestedTangleManager& orig) = delete;
        virtual ~NestedTangleManager();

        virtual void addTangle(const keto::asn1::HashHelper& tangle);
        virtual std::vector<keto::asn1::HashHelper> getActiveTangles();
        virtual void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);
        //virtual keto::asn1::HashHelper getTangleHash();
        virtual keto::asn1::HashHelper getParentHash();
        //virtual keto::asn1::HashHelper selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id);
        virtual keto::asn1::HashHelper selectParentHash();
        virtual void setCurrentTangle(const keto::asn1::HashHelper& tangle);

    private:
        BlockChainMetaPtr blockChainMetaPtr;
        BlockChainTangleMetaPtr activeTangle;


    };

    friend class BlockChainStore;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockChain(const BlockChain& orig) = delete;
    virtual ~BlockChain();

    bool requireGenesis();
    void applyDirtyTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr, const BlockChainCallback& callback);
    bool writeBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockBuilder, const BlockChainCallback& callback);
    bool writeBlock(const SignedBlockBuilderPtr& signedBlockBuilderPtr, const BlockChainCallback& callback);

    keto::asn1::HashHelper getParentHash();
    keto::asn1::HashHelper getParentHash(const keto::asn1::HashHelper& transactionHash);
    BlockChainMetaPtr getBlockChainMeta();


    // block chain cache
    static void initCache();
    static void clearCache();
    static void finCache();


    std::vector<keto::asn1::HashHelper> getLastBlockHashs();
    keto::proto::SignedBlockBatchMessage requestBlocks(const std::vector<keto::asn1::HashHelper>& tangledHashes);
    bool processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage, const BlockChainCallback& callback);
    bool processBlockSyncResponse(const keto::proto::SignedBlockBatch& signedBlockBatch, const BlockChainCallback& callback);

    keto::proto::AccountChainTangle getAccountBlockTangle(const keto::proto::AccountChainTangle& accountChainTangle);
    bool getAccountTangle(const keto::asn1::HashHelper& accountHash, keto::asn1::HashHelper& tangeHash);

    std::vector<keto::asn1::HashHelper> getActiveTangles();
    void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);
    void setCurrentTangle(const keto::asn1::HashHelper& tangle);


private:
    std::recursive_mutex classMutex;
    bool inited;
    bool masterChain;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    BlockResourceManagerPtr blockResourceManagerPtr;
    BlockChainMetaPtr blockChainMetaPtr;
    TangleManagerInterfacePtr tangleManagerInterfacePtr;
    //BlockChainTangleMetaPtr activeTangle;
    std::vector<BlockChainPtr> sideChains;


    BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
               BlockResourceManagerPtr blockResourceManagerPtr);
    BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
               BlockResourceManagerPtr blockResourceManagerPtr,const std::vector<uint8_t>& id);

    //keto::asn1::HashHelper selectParentHash();
    //keto::asn1::HashHelper selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id);
    //keto::asn1::HashHelper getTangleHash();

    void load(const std::vector<uint8_t>& id);
    void persist();

    BlockChainPtr getChildPtr(const keto::asn1::HashHelper& parentHash);
    BlockChainPtr getChildByTransactionId(const keto::asn1::HashHelper& parentHash);


    bool writeBlock(const SignedBlockWrapperProtoHelperPtr& signedBlockWrapperProtoHelperPtr, const BlockChainCallback& callback);
    bool writeBlock(BlockResourcePtr resource, SignedBlock& signedBlock, const BlockChainCallback& callback);
    void broadcastBlock(const keto::block_db::SignedBlockWrapperProtoHelper& signedBlockWrapperProtoHelper);


    keto::proto::SignedBlockBatch getBlockBatch(keto::asn1::HashHelper hash, BlockResourcePtr resource);
    keto::proto::SignedBlockWrapper getBlock(keto::asn1::HashHelper hash, BlockResourcePtr resource);

    bool accountExists(const keto::asn1::HashHelper& accountHash);
};

}
}

#endif //KETO_BLOCKCHAIN_HPP
