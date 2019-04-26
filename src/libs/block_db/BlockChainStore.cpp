/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockChainStore.cpp
 * Author: ubuntu
 * 
 * Created on February 23, 2018, 10:19 AM
 */

#include <string>
#include <iostream>
#include <sstream>

#include "keto/block_db/BlockChainStore.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/Constants.hpp"
#include "keto/server_common/TransactionHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockResource.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "include/keto/block_db/Exception.hpp"


namespace keto {
namespace block_db {
    
std::string BlockChainStore::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

static std::shared_ptr<BlockChainStore> singleton;
    
BlockChainStore::BlockChainStore() {
    BlockChain::initCache();
    dbManagerPtr = std::shared_ptr<keto::rocks_db::DBManager>(
            new keto::rocks_db::DBManager(Constants::DB_LIST));
    blockResourceManagerPtr  =  BlockResourceManagerPtr(
            new BlockResourceManager(dbManagerPtr));
}

BlockChainStore::~BlockChainStore() {
    this->masterChain.reset();
    blockResourceManagerPtr.reset();
    dbManagerPtr.reset();
    BlockChain::finCache();
}

std::shared_ptr<BlockChainStore> BlockChainStore::init() {
    if (singleton) {
        return singleton;
    }
    return singleton = std::shared_ptr<BlockChainStore>(new BlockChainStore());
}

void BlockChainStore::fin() {
    singleton.reset();
}

std::shared_ptr<BlockChainStore> BlockChainStore::getInstance() {
    return singleton;
}

void BlockChainStore::load() {
    if (this->masterChain) {
        return;
    }
    this->masterChain = BlockChainPtr(new BlockChain(dbManagerPtr,blockResourceManagerPtr));
}

bool BlockChainStore::requireGenesis() {
    /*BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    keto::asn1::HashHelper parentHash(Constants::GENESIS_KEY,keto::common::HEX);
    keto::crypto::SecureVector vector = 
            parentHash.operator keto::crypto::SecureVector();
    keto::rocks_db::SliceHelper keyHelper(keto::crypto::SecureVectorUtils().copyFromSecure(vector));
    rocksdb::Transaction* childTransaction = resource->getTransaction(Constants::CHILD_INDEX);
    rocksdb::ReadOptions readOptions;
    std::string value;
    auto status = childTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status || rocksdb::Status::NotFound() == status) {
        return true;
    }
    if (value.empty()) {
        return true;
    }
    //std::cout << "The value is [" << value << "]" << std::endl;
    // init the block and header 
    this->getBlockCount();
    this->getParentHash();
    return false; */
    return this->masterChain->requireGenesis();
}

void BlockChainStore::applyDirtyTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr, const BlockChainCallback& callback) {
    return this->masterChain->applyDirtyTransaction(transactionMessageHelperPtr, callback);
}

void BlockChainStore::writeBlock(const SignedBlockBuilderPtr& signedBlock, const BlockChainCallback& callback) {
    return this->masterChain->writeBlock(signedBlock,callback);
}

void BlockChainStore::writeBlock(const keto::proto::SignedBlockWrapperMessage& signedBlock, const BlockChainCallback& callback) {
    return this->masterChain->writeBlock(signedBlock,callback);
}

/*
void BlockChainStore::writeBlock(SignedBlock& signedBlock) {
    
    // write the block
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::Transaction* childTransaction = resource->getTransaction(Constants::CHILD_INDEX);
    rocksdb::Transaction* transactionTransaction = resource->getTransaction(Constants::TRANSACTIONS_INDEX);
    std::shared_ptr<keto::asn1::SerializationHelper<SignedBlock>> serializationHelperPtr =
            std::make_shared<keto::asn1::SerializationHelper<SignedBlock>>(
                &signedBlock,&asn_DEF_SignedBlock);
    keto::rocks_db::SliceHelper valueHelper((const std::vector<uint8_t>)(*serializationHelperPtr));
    keto::rocks_db::SliceHelper blockHashHelper(keto::crypto::SecureVectorUtils().copyFromSecure(
        keto::asn1::HashHelper(signedBlock.hash)));
    keto::rocks_db::SliceHelper parentHashHelper(keto::crypto::SecureVectorUtils().copyFromSecure(
        keto::asn1::HashHelper(signedBlock.parent)));
    blockTransaction->Put(blockHashHelper,valueHelper);
    childTransaction->Put(parentHashHelper,blockHashHelper);
    
    // update the key tracking the parent key
    keto::rocks_db::SliceHelper parentKeyHelper((std::string(Constants::PARENT_KEY)));
    blockTransaction->Put(parentKeyHelper,blockHashHelper);
    
    parentBlock = keto::asn1::HashHelper(blockHashHelper);
    
    // write the block count
    if (this->blockCount == -1) {
        this->blockCount = 0;
    }
    keto::rocks_db::SliceHelper blockCountKeyHelper((std::string(Constants::BLOCK_COUNT)));
    std::stringstream ss;
    ss << ++this->blockCount;
    keto::rocks_db::SliceHelper blockCountValueHelper(ss.str());
    blockTransaction->Put(blockCountKeyHelper,blockCountValueHelper);

    // setup the transaction indexing for the block
    for (int index = 0; index < signedBlock.block.transactions.list.count; index++) {
        transactionTransaction->Put(keto::rocks_db::SliceHelper(
            keto::crypto::SecureVectorUtils().copyFromSecure(
            keto::asn1::HashHelper(
            signedBlock.block.transactions.list.array[index]->transactionHash))),blockHashHelper);
    }

}*/

keto::asn1::HashHelper BlockChainStore::getParentHash() {
    /*if (this->parentBlock.empty()) {
        BlockResourcePtr resource = blockResourceManagerPtr->getResource();
        keto::rocks_db::SliceHelper keyHelper((std::string(Constants::PARENT_KEY)));
        std::string value;
        rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
        rocksdb::ReadOptions readOptions;
        if (rocksdb::Status::OK() != blockTransaction->Get(readOptions,keyHelper,&value)) {
            BOOST_THROW_EXCEPTION(keto::block_db::InvalidParentKeyIdentifierException());
        }
        return this->parentBlock = keto::asn1::HashHelper(value);
    } else {
        return this->parentBlock;
    }*/
    return this->masterChain->getParentHash();
}

keto::asn1::HashHelper BlockChainStore::getParentHash(const keto::asn1::HashHelper& transactionHash) {
    return this->masterChain->getParentHash(transactionHash);
}

}
}
