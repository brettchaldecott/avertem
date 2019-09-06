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
    return this->masterChain->requireGenesis();
}

void BlockChainStore::applyDirtyTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr, const BlockChainCallback& callback) {
    return this->masterChain->applyDirtyTransaction(transactionMessageHelperPtr, callback);
}

bool BlockChainStore::writeBlock(const SignedBlockBuilderPtr& signedBlock, const BlockChainCallback& callback) {
    return this->masterChain->writeBlock(signedBlock,callback);
}

bool BlockChainStore::writeBlock(const keto::proto::SignedBlockWrapperMessage& signedBlock, const BlockChainCallback& callback) {
    if (!masterChain) {
        KETO_LOG_DEBUG << "The block chain has not been initialized yet, ignore new blocks";
        return false;
    }
    return this->masterChain->writeBlock(signedBlock,callback);
}

std::vector<keto::asn1::HashHelper> BlockChainStore::getLastBlockHashs() {
    if (!masterChain) {
        KETO_LOG_DEBUG << "The block chain has not been initialized yet, ignore new blocks";
        return std::vector<keto::asn1::HashHelper>();
    }
    return this->masterChain->getLastBlockHashs();
}

keto::proto::SignedBlockBatchMessage BlockChainStore::requestBlocks(const std::vector<keto::asn1::HashHelper>& tangledHashes) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    // if the tangled hases size is zero we have to start from the genesis block
    std::vector<keto::asn1::HashHelper> hashes = tangledHashes;
    if (!hashes.size()) {
        for (int index = 0; index < this->masterChain->getBlockChainMeta()->tangleCount(); index++) {
            keto::asn1::HashHelper tangleHash = this->masterChain->getBlockChainMeta()->getTangleEntry(index)->getHash();
            KETO_LOG_DEBUG<< "[BlockChainStore::requestBlocks][" << index << "] Client is requesting the complete chain using the starting point of :  " <<
                          tangleHash.getHash(keto::common::StringEncoding::HEX);
            hashes.push_back(tangleHash);
        }

    }
    return this->masterChain->requestBlocks(hashes);
}

bool BlockChainStore::processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage, const BlockChainCallback& callback) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }

    return this->masterChain->processBlockSyncResponse(signedBlockBatchMessage,callback);
}

keto::proto::AccountChainTangle BlockChainStore::getAccountBlockTangle(const keto::proto::AccountChainTangle& accountChainTangle) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }

    return this->masterChain->getAccountBlockTangle(accountChainTangle);
}

bool BlockChainStore::getAccountTangle(const keto::asn1::HashHelper& accountHash, keto::asn1::HashHelper& tangleHash) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->getAccountTangle(accountHash,tangleHash);
}

bool BlockChainStore::containsTangleInfo(const keto::asn1::HashHelper& tangleHash) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->containsTangleInfo(tangleHash);
}

BlockChainTangleMetaPtr  BlockChainStore::getTangleInfo(const keto::asn1::HashHelper& tangleHash) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->getTangleInfo(tangleHash);
}


keto::asn1::HashHelper BlockChainStore::getParentHash() {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->getParentHash();
}

keto::asn1::HashHelper BlockChainStore::getParentHash(const keto::asn1::HashHelper& transactionHash) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->getParentHash(transactionHash);
}


std::vector<keto::asn1::HashHelper> BlockChainStore::getActiveTangles() {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->getActiveTangles();
}

keto::asn1::HashHelper BlockChainStore::getGrowTangle() {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    return this->masterChain->getGrowTangle();
}

void BlockChainStore::setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    this->masterChain->setActiveTangles(tangles);
}

void BlockChainStore::setCurrentTangle(const keto::asn1::HashHelper& tangle) {
    if (!masterChain) {
        BOOST_THROW_EXCEPTION(keto::block_db::ChainNotInitializedException());
    }
    this->masterChain->setCurrentTangle(tangle);
}

}
}
