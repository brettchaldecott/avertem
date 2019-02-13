//
// Created by Brett Chaldecott on 2019/02/06.
//

#include <Block.h>
#include <keto/server_common/StringUtils.hpp>
#include "BlockChain.pb.h"

#include "keto/rocks_db/DBManager.hpp"
#include "keto/server_common/TransactionHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockResource.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"


#include "keto/block_db/BlockChain.hpp"
#include "keto/block_db/Exception.hpp"
#include "keto/block_db/Constants.hpp"
#include "keto/transaction_common/AccountTransactionInfoProtoHelper.hpp"

namespace keto {
namespace block_db {

std::string BlockChain::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockChain::~BlockChain() {

}

bool BlockChain::requireGenesis() {
    return inited;
}

void BlockChain::writeBlock(const SignedBlockBuilderPtr& signedBlock, const BlockChainCallback& callback) {
    writeBlock(*signedBlock->signedBlock,callback);
    for (SignedBlockBuilderPtr nestedBlock: signedBlock->nestedBlocks) {
        BlockChainPtr childPtr;
        if (nestedBlock->getParentHash() == keto::block_db::Constants::GENESIS_HASH) {
            childPtr = BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
                    nestedBlock->getHash()));
        }  else {
            childPtr = getChildPtr(nestedBlock->getParentHash());
        }
        childPtr->writeBlock(nestedBlock,callback);
    }
}

keto::asn1::HashHelper BlockChain::getParentHash() {
    return this->activeTangle->getLastBlockHash();
}

keto::asn1::HashHelper BlockChain::getParentHash(const keto::asn1::HashHelper& transactionHash) {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* transaction = resource->getTransaction(Constants::TRANSACTIONS_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((const std::vector<uint8_t>)transactionHash);
    std::string value;
    auto status = transaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status || rocksdb::Status::NotFound() == status) {
        BOOST_THROW_EXCEPTION(keto::block_db::InvalidTransactionHashException());
    }

    keto::proto::TransactionMeta transactionMeta;
    transactionMeta.ParseFromString(value);
    return transactionMeta.block_hash();
}

BlockChainMetaPtr BlockChain::getBlockChainMeta() {
    return this->blockChainMetaPtr;
}


BlockChain::BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
        BlockResourceManagerPtr blockResourceManagerPtr) :
        inited(false), dbManagerPtr(dbManagerPtr), blockResourceManagerPtr(blockResourceManagerPtr) {
    load(Constants::MASTER_CHAIN_HASH);
}

BlockChain::BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
        BlockResourceManagerPtr blockResourceManagerPtr,const std::vector<uint8_t>& id) :
        inited(false), dbManagerPtr(dbManagerPtr), blockResourceManagerPtr(blockResourceManagerPtr) {
    load(id);
}

keto::asn1::HashHelper BlockChain::selectParentHash() {
    if (activeTangle) {
        return activeTangle->getLastBlockHash();
    }
    activeTangle = this->blockChainMetaPtr->selectTangleEntry();
    return activeTangle->getLastBlockHash();
}

keto::asn1::HashHelper BlockChain::selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id) {
    this->activeTangle =
            this->blockChainMetaPtr->getTangleEntryByLastBlock(id);
    if (!this->activeTangle) {
        BOOST_THROW_EXCEPTION(keto::block_db::InvalidLastBlockHashException());
    }
    return this->activeTangle->getHash();
}

void BlockChain::writeBlock(SignedBlock& signedBlock, const BlockChainCallback& callback) {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::Transaction* childTransaction = resource->getTransaction(Constants::CHILD_INDEX);
    rocksdb::Transaction* transactionTransaction = resource->getTransaction(Constants::TRANSACTIONS_INDEX);
    std::shared_ptr<keto::asn1::SerializationHelper<SignedBlock>> serializationHelperPtr =
            std::make_shared<keto::asn1::SerializationHelper<SignedBlock>>(
                    &signedBlock,&asn_DEF_SignedBlock);
    keto::asn1::HashHelper blockHash(signedBlock.hash);
    keto::rocks_db::SliceHelper blockHashHelper((const std::vector<uint8_t>)blockHash);
    keto::asn1::HashHelper parentHash(signedBlock.parent);
    keto::rocks_db::SliceHelper parentHashHelper((const std::vector<uint8_t>)parentHash);


    callback.prePersistBlock(blockChainMetaPtr->getHashId(),signedBlock);

    // setup the transaction indexing for the block
    for (int index = 0; index < signedBlock.block.transactions.list.count; index++) {
        TransactionWrapper_t* transactionWrapper = signedBlock.block.transactions.list.array[index];
        if (!inited && index == 0) {
            keto::transaction_common::TransactionWrapperHelper transactionWrapperHelper(transactionWrapper,false);
            this->blockChainMetaPtr->setCreated(transactionWrapperHelper.getSignedTransaction()->getTransaction()->getDate());
            this->blockChainMetaPtr->setEncrypted(transactionWrapperHelper.getSignedTransaction()->getTransaction()->isEncrypted());
        }
        callback.prePersistTransaction(blockChainMetaPtr->getHashId(),
                                       signedBlock, *transactionWrapper);
        keto::asn1::HashHelper transactionHash(transactionWrapper->transactionHash);

        keto::proto::TransactionMeta transactionMeta;
        transactionMeta.set_transaction_hash(transactionHash);
        transactionMeta.set_block_hash(blockHash);
        transactionMeta.set_block_chain_hash(this->blockChainMetaPtr->getHashId());
        std::string value;
        transactionMeta.SerializeToString(&value);
        keto::rocks_db::SliceHelper transactionMetaHelper(value);

        transactionTransaction->Put(keto::rocks_db::SliceHelper(
                (const std::vector<uint8_t>)transactionHash),transactionMetaHelper);

        // post persist
        callback.postPersistTransaction(blockChainMetaPtr->getHashId(),
                                       signedBlock, *transactionWrapper);
    }

    keto::proto::BlockWrapper blockWrapper;
    blockWrapper.set_asn1_block(keto::server_common::VectorUtils().copyVectorToString(
            (const std::vector<uint8_t>)(*serializationHelperPtr)));

    keto::proto::BlockMeta blockMeta;
    blockMeta.set_block_hash((const std::string&)blockHash);
    blockMeta.set_block_parent_hash((const std::string&)parentHash);
    blockMeta.set_chain_id((const std::string&)this->blockChainMetaPtr->getHashId());
    blockWrapper.set_allocated_block_meta(&blockMeta);

    std::string blockWrapperStr;
    blockWrapper.SerializeToString(&blockWrapperStr);
    keto::rocks_db::SliceHelper valueHelper(blockWrapperStr);


    blockTransaction->Put(blockHashHelper,valueHelper);
    childTransaction->Put(parentHashHelper,blockHashHelper);

    // retrieve the parent hash
    BlockChainTangleMetaPtr blockChainTangleMetaPtr =
            this->blockChainMetaPtr->getTangleEntryByLastBlock(parentHash);
    if (!blockChainTangleMetaPtr) {
        blockChainTangleMetaPtr = this->blockChainMetaPtr->addTangle(blockHash);
    } else {
        blockChainTangleMetaPtr->setLastBlockHash(blockHash);
    }
    blockChainTangleMetaPtr->setLastModified(keto::asn1::TimeHelper(signedBlock.date));

    // update the key tracking the parent key
    keto::rocks_db::SliceHelper parentKeyHelper((std::string(Constants::PARENT_KEY)));
    blockTransaction->Put(parentKeyHelper,blockHashHelper);

    callback.postPersistBlock(blockChainMetaPtr->getHashId(),signedBlock);

    persist();
}


void BlockChain::load(const std::vector<uint8_t>& id) {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* blockMetaTransaction =  resource->getTransaction(Constants::BLOCK_META_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper(id);
    std::string value;
    auto status = blockMetaTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status || rocksdb::Status::NotFound() == status) {
        this->blockChainMetaPtr = BlockChainMetaPtr(
                new BlockChainMeta(id));
    } else {
        this->inited = true;
        keto::proto::BlockChainMeta blockChainMeta;
        blockChainMeta.ParseFromString(value);
        this->blockChainMetaPtr = BlockChainMetaPtr(
                new BlockChainMeta(blockChainMeta));
    }
}

void BlockChain::persist() {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* blockMetaTransaction =  resource->getTransaction(Constants::BLOCK_META_INDEX);
    keto::rocks_db::SliceHelper keyHelper((const std::vector<uint8_t >)this->blockChainMetaPtr->getHashId());
    keto::rocks_db::SliceHelper metaData((const std::string)*this->blockChainMetaPtr);
    blockMetaTransaction->Put(keyHelper,metaData);
    inited = true;
}


BlockChainPtr BlockChain::getChildPtr(const keto::asn1::HashHelper& parentHash) {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::ReadOptions readOptions;
    std::string value;
    keto::rocks_db::SliceHelper parentHashSlice((const std::vector<uint8_t>)parentHash);
    auto status = blockTransaction->Get(readOptions,parentHashSlice,&value);
    if (rocksdb::Status::OK() != status || rocksdb::Status::NotFound() == status) {
        BOOST_THROW_EXCEPTION(keto::block_db::InvalidParentHashIdentifierException());
    }
    keto::proto::BlockWrapper blockWrapper;
    blockWrapper.ParseFromString(value);
    return BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
            keto::server_common::VectorUtils().copyStringToVector(blockWrapper.block_meta().chain_id())));
}

}
}
