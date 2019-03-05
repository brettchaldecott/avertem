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

#include "SignedBlock.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockChain.hpp"
#include "keto/block_db/SignedBlockBuilder.hpp"
#include "keto/block_db/BlockChainCallback.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"


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
    void writeBlock(const SignedBlockBuilderPtr& signedBlock, const BlockChainCallback& callback);
    keto::asn1::HashHelper getParentHash();
    keto::asn1::HashHelper getParentHash(const keto::asn1::HashHelper& transactionHash);

private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    BlockResourceManagerPtr blockResourceManagerPtr;
    BlockChainPtr masterChain;
};


}
}


#endif /* BLOCKCHAINSTORE_HPP */

