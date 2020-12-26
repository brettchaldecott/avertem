/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockChainStore.hpp
 * Author: ubuntu
 *
 * Created on February 23, 2018, 10:19 AM
 */

#ifndef BLOCKCHAINSTORE_HPP
#define BLOCKCHAINSTORE_HPP

#include <string>
#include <memory>
#include <mutex>

#include "SignedBlock.h"
#include "BlockChain.pb.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockChain.hpp"
#include "keto/block_db/SignedBlockBuilder.hpp"
#include "keto/block_db/BlockChainCallback.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/chain_query_common/BlockQueryProtoHelper.hpp"
#include "keto/chain_query_common/BlockResultSetProtoHelper.hpp"
#include "keto/chain_query_common/TransactionQueryProtoHelper.hpp"
#include "keto/chain_query_common/TransactionResultSetProtoHelper.hpp"


namespace keto {
namespace block_db {

class BlockChainStore {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockChainStore();
    BlockChainStore(const BlockChainStore& orig) = delete;
    virtual ~BlockChainStore();
    
    static std::shared_ptr<BlockChainStore> init();
    static void fin();
    static std::shared_ptr<BlockChainStore> getInstance();


    void load();
    bool requireGenesis();
    void applyDirtyTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr, const BlockChainCallback& callback);
    bool writeBlock(const SignedBlockBuilderPtr& signedBlock, const BlockChainCallback& callback);
    bool writeBlock(const keto::proto::SignedBlockWrapperMessage& signedBlock, const BlockChainCallback& callback);
    std::vector<keto::asn1::HashHelper> getLastBlockHashs();
    bool requestBlocks(const std::vector<keto::asn1::HashHelper>& tangledHashes, keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage);
    bool processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage, const BlockChainCallback& callback);
    bool processProducerEnding(const keto::block_db::SignedBlockWrapperMessageProtoHelper& signedBlockWrapperMessageProtoHelper);

    // tangle methods
    keto::proto::AccountChainTangle getAccountBlockTangle(const keto::proto::AccountChainTangle& accountChainTangle);
    bool getAccountTangle(const keto::asn1::HashHelper& accountHash, keto::asn1::HashHelper& tangleHash);
    bool containsTangleInfo(const keto::asn1::HashHelper& tangleHash);
    BlockChainTangleMetaPtr getTangleInfo(const keto::asn1::HashHelper& tangleHash);

    keto::asn1::HashHelper getParentHash();
    keto::asn1::HashHelper getParentHash(const keto::asn1::HashHelper& transactionHash);

    std::vector<keto::asn1::HashHelper> getActiveTangles();
    keto::asn1::HashHelper getGrowTangle();
    void clearActiveTangles();
    void setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);
    void setCurrentTangle(const keto::asn1::HashHelper& tangle);

    keto::chain_query_common::BlockResultSetProtoHelperPtr performBlockQuery(const keto::chain_query_common::BlockQueryProtoHelper& blockQueryProtoHelper);
    keto::chain_query_common::TransactionResultSetProtoHelperPtr performTransactionQuery(const keto::chain_query_common::TransactionQueryProtoHelper& transactionQueryProtoHelper);

private:
    std::recursive_mutex classMutex;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;

    BlockResourceManagerPtr blockResourceManagerPtr;
    BlockChainPtr masterChain;
};


}
}


#endif /* BLOCKCHAINSTORE_HPP */

