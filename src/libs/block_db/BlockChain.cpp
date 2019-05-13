//
// Created by Brett Chaldecott on 2019/02/06.
//

#include <Block.h>
#include <keto/server_common/StringUtils.hpp>
#include <keto/server_common/Events.hpp>
#include "BlockChain.pb.h"

#include "keto/rocks_db/DBManager.hpp"
#include "keto/server_common/TransactionHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/DeserializationHelper.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockResource.hpp"
#include "keto/rocks_db/SliceHelper.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/key_store_utils/EncryptionResponseProtoHelper.hpp"
#include "keto/key_store_utils/EncryptionRequestProtoHelper.hpp"

#include "keto/block_db/BlockChain.hpp"
#include "keto/block_db/Exception.hpp"
#include "keto/block_db/Constants.hpp"
#include "keto/transaction_common/AccountTransactionInfoProtoHelper.hpp"

#include "keto/block_db/SignedBlockWrapperProtoHelper.hpp"
#include "keto/block_db/SignedBlockWrapperMessageProtoHelper.hpp"

namespace keto {
namespace block_db {

static BlockChain::BlockChainCachePtr singleton;

BlockChain::BlockChainCache::BlockChainCache() {

}

BlockChain::BlockChainCache::~BlockChainCache() {

}

BlockChain::BlockChainCachePtr BlockChain::BlockChainCache::createInstance() {
    return singleton = BlockChain::BlockChainCachePtr(new BlockChain::BlockChainCache());
}

BlockChain::BlockChainCachePtr BlockChain::BlockChainCache::getInstance() {
    return singleton;
}

void BlockChain::BlockChainCache::clear() {
    singleton->clearCache();
}

void BlockChain::BlockChainCache::fin() {
    singleton.reset();
}

BlockChainPtr BlockChain::BlockChainCache::getBlockChain(const keto::asn1::HashHelper& parentHash) {
    return transactionIdBlockChainMap[parentHash];
}

void BlockChain::BlockChainCache::addBlockChain(const keto::asn1::HashHelper& transactionHash, const BlockChainPtr& blockChainPtr) {
    transactionIdBlockChainMap[transactionHash] = blockChainPtr;
}

void BlockChain::BlockChainCache::clearCache() {
    transactionIdBlockChainMap.clear();
}

std::string BlockChain::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockChain::~BlockChain() {

}

bool BlockChain::requireGenesis() {
    return !inited;
}


void BlockChain::applyDirtyTransaction(keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelperPtr, const BlockChainCallback& callback) {

    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr = transactionMessageHelperPtr->getTransactionWrapper();
    callback.applyDirtyTransaction(this->blockChainMetaPtr->getHashId(),*transactionWrapperHelperPtr);
    for (keto::transaction_common::TransactionMessageHelperPtr& nestedTransaction: transactionMessageHelperPtr->getNestedTransactions()) {
        BlockChainPtr childPtr;
        keto::transaction_common::TransactionWrapperHelperPtr nestedTransactionWrapperHelperPtr = nestedTransaction->getTransactionWrapper();
        if (nestedTransactionWrapperHelperPtr->getParentHash() == keto::block_db::Constants::GENESIS_HASH) {
            // create a new block chain using the transaction hash
            childPtr = BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
                                                    nestedTransaction->getTransactionWrapper()->getHash()));
        }  else {
            childPtr = getChildByTransactionId(nestedTransactionWrapperHelperPtr->getParentHash());
        }
        childPtr->applyDirtyTransaction(nestedTransaction,callback);
        BlockChain::BlockChainCache::getInstance()->addBlockChain(nestedTransactionWrapperHelperPtr->getHash(),childPtr);
    }
}


keto::asn1::HashHelper BlockChain::getParentHash() {
    if (!this->activeTangle) {
        return selectParentHash();
    }
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


// block chain cache
void BlockChain::initCache() {
    BlockChain::BlockChainCache::createInstance();
}

void BlockChain::clearCache() {
    BlockChain::BlockChainCache::clear();
}

void BlockChain::finCache() {
    BlockChain::BlockChainCache::fin();
}

BlockChain::BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
        BlockResourceManagerPtr blockResourceManagerPtr) :
        inited(false), masterChain(true), dbManagerPtr(dbManagerPtr), blockResourceManagerPtr(blockResourceManagerPtr) {
    load(Constants::MASTER_CHAIN_HASH);
}

BlockChain::BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
        BlockResourceManagerPtr blockResourceManagerPtr,const std::vector<uint8_t>& id) :
        inited(false), masterChain(false), dbManagerPtr(dbManagerPtr), blockResourceManagerPtr(blockResourceManagerPtr) {
    load(id);
}

keto::asn1::HashHelper BlockChain::selectParentHash() {
    if (activeTangle) {
        return activeTangle->getLastBlockHash();
    }
    activeTangle = this->blockChainMetaPtr->selectTangleEntry();
    if (!activeTangle) {
        return keto::asn1::HashHelper();
    }
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

void BlockChain::writeBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage, const BlockChainCallback& callback) {
    SignedBlockWrapperMessageProtoHelper signedBlockWrapperMessageProtoHelper(signedBlockWrapperMessage);

    SignedBlockWrapperProtoHelperPtr signedBlockWrapperProtoHelperPtr =
            signedBlockWrapperMessageProtoHelper.getSignedBlockWrapper();

    writeBlock(signedBlockWrapperProtoHelperPtr, callback);
    broadcastBlock(*signedBlockWrapperProtoHelperPtr);
}

void BlockChain::writeBlock(const SignedBlockWrapperProtoHelperPtr& signedBlockWrapperProtoHelperPtr, const BlockChainCallback& callback) {
    SignedBlock& signedBlock = *signedBlockWrapperProtoHelperPtr;

    BlockResourcePtr resource = blockResourceManagerPtr->getResource();

    // check for a duplicate block
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((const std::vector<uint8_t>)signedBlockWrapperProtoHelperPtr->getHash());
    std::string value;

    auto status = blockTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() != status) {
        // ignore as the block already exists
        return;
    }

    writeBlock(resource, signedBlock, callback);

    rocksdb::Transaction* nestedTransaction = resource->getTransaction(Constants::NESTED_INDEX);

    // handle the nested blocks
    keto::proto::NestedBlockMeta nestedBlockMeta;
    nestedBlockMeta.set_block_hash(blockChainMetaPtr->getHashId());
    for (SignedBlockWrapperProtoHelperPtr signedNestedBlock: signedBlockWrapperProtoHelperPtr->getNestedBlocks()) {
        BlockChainPtr childPtr;

        if (signedNestedBlock->getParentHash() == keto::block_db::Constants::GENESIS_HASH) {
            // create a new block chain using the transaction hash
            childPtr = BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
                                                    signedNestedBlock->getFirstTransactionHash()));
        }  else {
            childPtr = getChildPtr(signedNestedBlock->getParentHash());
        }
        childPtr->writeBlock(signedNestedBlock,callback);
        *nestedBlockMeta.add_nested_hashs() = signedNestedBlock->getHash();
    }

    keto::asn1::HashHelper blockHash(signedBlock.hash);
    keto::rocks_db::SliceHelper blockHashHelper((const std::vector<uint8_t>)blockHash);

    std::string nestedBlockStr;
    nestedBlockMeta.SerializeToString(&nestedBlockStr);
    keto::rocks_db::SliceHelper nestedSliceHelper(nestedBlockStr);
    nestedTransaction->Put(blockHashHelper,nestedSliceHelper);

    persist();

}

void BlockChain::writeBlock(const SignedBlockBuilderPtr& signedBlockBuilderPtr, const BlockChainCallback& callback) {
    SignedBlock& signedBlock = *signedBlockBuilderPtr;

    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    writeBlock(resource, signedBlock, callback);

    rocksdb::Transaction* nestedTransaction = resource->getTransaction(Constants::NESTED_INDEX);

    // handle the nested blocks
    keto::proto::NestedBlockMeta nestedBlockMeta;
    nestedBlockMeta.set_block_hash(blockChainMetaPtr->getHashId());
    for (SignedBlockBuilderPtr nestedBlock: signedBlockBuilderPtr->getNestedBlocks()) {
        BlockChainPtr childPtr;
        if (nestedBlock->getParentHash() == keto::block_db::Constants::GENESIS_HASH) {
            // create a new block chain using the transaction hash
            childPtr = BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
                                                    nestedBlock->getFirstTransactionHash()));
        }  else {
            childPtr = getChildPtr(nestedBlock->getParentHash());
        }
        childPtr->writeBlock(nestedBlock,callback);
        *nestedBlockMeta.add_nested_hashs() = nestedBlock->getHash();
    }

    keto::asn1::HashHelper blockHash(signedBlock.hash);
    keto::rocks_db::SliceHelper blockHashHelper((const std::vector<uint8_t>)blockHash);

    std::string nestedBlockStr;
    nestedBlockMeta.SerializeToString(&nestedBlockStr);
    keto::rocks_db::SliceHelper nestedSliceHelper(nestedBlockStr);
    nestedTransaction->Put(blockHashHelper,nestedSliceHelper);

    persist();

    keto::block_db::SignedBlockWrapperProtoHelper signedBlockWrapperProtoHelper(signedBlockBuilderPtr);
    broadcastBlock(signedBlockWrapperProtoHelper);

}




void BlockChain::writeBlock(BlockResourcePtr resource, SignedBlock& signedBlock, const BlockChainCallback& callback) {

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

    std::vector<uint8_t> blockBytes = (const std::vector<uint8_t>)(*serializationHelperPtr);

    keto::proto::BlockMeta blockMeta;
    blockMeta.set_block_hash((const std::string&)blockHash);
    blockMeta.set_block_parent_hash((const std::string&)parentHash);
    blockMeta.set_chain_id((const std::string&)this->blockChainMetaPtr->getHashId());
    blockMeta.set_encrypted(this->blockChainMetaPtr->isEncrypted());

    if (this->blockChainMetaPtr->isEncrypted()) {
        std::cout << "Encrypt the block" << std::endl;
        keto::key_store_utils::EncryptionRequestProtoHelper encryptionRequestProtoHelper;
        encryptionRequestProtoHelper = blockBytes;
        keto::key_store_utils::EncryptionResponseProtoHelper encryptionResponseProtoHelper(
                keto::server_common::fromEvent<keto::proto::EncryptResponse>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptRequest>(
                        keto::server_common::Events::ENCRYPT_ASN1::ENCRYPT,encryptionRequestProtoHelper))));
        blockWrapper.set_asn1_block(encryptionResponseProtoHelper);
    } else {
        blockWrapper.set_asn1_block(keto::server_common::VectorUtils().copyVectorToString(
           blockBytes));
    }

    *blockWrapper.mutable_block_meta() = blockMeta;

    // store the block wrapper in the db
    std::string blockWrapperStr;
    blockWrapper.SerializeToString(&blockWrapperStr);
    keto::rocks_db::SliceHelper valueHelper(blockWrapperStr);
    blockTransaction->Put(blockHashHelper,valueHelper);

    // add the children
    std::string childBlocks;
    rocksdb::ReadOptions readOptions;
    auto status = childTransaction->Get(readOptions, parentHashHelper, &childBlocks);
    keto::proto::BlockChildren blockChildren;
    if (rocksdb::Status::OK() == status && rocksdb::Status::NotFound() != status) {
        keto::proto::BlockChildren blockChildren;
        blockChildren.ParseFromString(childBlocks);
    }
    blockChildren.add_hashes((std::string)blockHash);
    std::string blockChildrenStr;
    blockChildren.SerializeToString(&blockChildrenStr);
    keto::rocks_db::SliceHelper blockChildrenHelper(blockChildrenStr);
    childTransaction->Put(parentHashHelper,blockChildrenHelper);

    // retrieve the parent hash
    BlockChainTangleMetaPtr blockChainTangleMetaPtr =
            this->blockChainMetaPtr->getTangleEntryByLastBlock(parentHash);
    if (!blockChainTangleMetaPtr) {
        std::cout << "Add a new block chain tangle" << std::endl;
        blockChainTangleMetaPtr = this->blockChainMetaPtr->addTangle(blockHash);

    } else {
        std::cout << "Update the last block hash" << std::endl;
        blockChainTangleMetaPtr->setLastBlockHash(blockHash);
    }
    blockChainTangleMetaPtr->setLastModified(keto::asn1::TimeHelper(signedBlock.date));

    // update the key tracking the parent key
    keto::rocks_db::SliceHelper parentKeyHelper((std::string(Constants::PARENT_KEY)));
    blockTransaction->Put(parentKeyHelper,blockHashHelper);

    callback.postPersistBlock(blockChainMetaPtr->getHashId(),signedBlock);



}


std::vector<keto::asn1::HashHelper> BlockChain::getLastBlockHashs() {
    std::vector<keto::asn1::HashHelper> result;
    for (int count = 0; count < this->blockChainMetaPtr->tangleCount(); count++) {
        result.push_back(this->blockChainMetaPtr->getTangleEntry(count)->getLastBlockHash());
    }

    return result;
}

keto::proto::SignedBlockBatchMessage BlockChain::requestBlocks(const std::vector<keto::asn1::HashHelper>& tangledHashes) {
    keto::proto::SignedBlockBatchMessage result;

    BlockResourcePtr resource = blockResourceManagerPtr->getResource();

    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);


    for (keto::asn1::HashHelper hash : tangledHashes) {
        *result.add_tangle_batches() = getBlockBatch(hash, resource);
    }

    return result;
}

bool BlockChain::processBlockSyncResponse(const keto::proto::SignedBlockBatchMessage& signedBlockBatchMessage, const BlockChainCallback& callback) {
    bool complete = true;
    for (int index = 0; index < signedBlockBatchMessage.tangle_batches_size(); index++) {
        const keto::proto::SignedBlockBatch& signedBlockBatch = signedBlockBatchMessage.tangle_batches(index);
        if (!processBlockSyncResponse(signedBlockBatch,callback) && complete) {
            complete = false;
        }
    }
    return complete;
}


bool BlockChain::processBlockSyncResponse(const keto::proto::SignedBlockBatch& signedBlockBatch, const BlockChainCallback& callback) {
    bool complete = true;
    for (int index = 0; index < signedBlockBatch.blocks_size(); index++) {
        complete = false;
        this->writeBlock(signedBlockBatch.blocks(index),callback);
    }
    for (int index = 0; index < signedBlockBatch.tangle_batches_size(); index++) {
        if (!processBlockSyncResponse(signedBlockBatch.tangle_batches(index),callback) && complete) {
            complete = false;
        }
    }
    return complete;
}


keto::proto::SignedBlockBatch BlockChain::getBlockBatch(keto::asn1::HashHelper hash, BlockResourcePtr resource) {
    keto::proto::SignedBlockBatch signedBlockBatch;
    signedBlockBatch.set_start_block_id((std::string)hash);
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::Transaction* childTransaction = resource->getTransaction(Constants::CHILD_INDEX);

    bool found = false;
    keto::asn1::HashHelper blockHash = hash;
    do {
        keto::proto::SignedBlockWrapper signedBlockWrapper = getBlock(blockHash, resource);

        keto::block_db::SignedBlockWrapperMessageProtoHelper signedBlockWrapperMessageProtoHelper(signedBlockWrapper);
        *signedBlockBatch.add_blocks() = (keto::proto::SignedBlockWrapperMessage)signedBlockWrapperMessageProtoHelper;
        if (signedBlockBatch.blocks_size() >= Constants::MAX_BLOCK_REQUEST) {
            break;
        }
        std::string value;
        rocksdb::ReadOptions readOptions;
        keto::rocks_db::SliceHelper blockHashHelper((const std::vector<uint8_t>)blockHash);
        auto status = childTransaction->Get(readOptions, blockHashHelper, &value);
        found = false;
        if (rocksdb::Status::OK() == status && rocksdb::Status::NotFound() != status) {
            keto::proto::BlockChildren blockChildren;
            blockChildren.ParseFromString(value);
            for (int index = 1; index < blockChildren.hashes_size(); index++) {
                *signedBlockBatch.add_tangle_batches() = getBlockBatch(blockChildren.hashes(index),resource);
            }

            signedBlockBatch.set_end_block_id(blockChildren.hashes(0));
            found = true;
        }
    } while(found);

    return signedBlockBatch;
}

keto::proto::SignedBlockWrapper BlockChain::getBlock(keto::asn1::HashHelper hash, BlockResourcePtr resource) {

    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::Transaction* nestedTransaction = resource->getTransaction(Constants::NESTED_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((std::vector<uint8_t>)hash);
    std::string value;

    auto status = blockTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status || rocksdb::Status::NotFound() == status) {
        // not found
        BOOST_THROW_EXCEPTION(keto::block_db::InvalidLastBlockHashException());
    }

    keto::proto::BlockWrapper blockWrapper;
    blockWrapper.ParseFromString(value);

    // decrypt the block
    if (this->blockChainMetaPtr->isEncrypted()) {
        std::cout << "Encrypt the block" << std::endl;
        keto::key_store_utils::EncryptionRequestProtoHelper encryptionRequestProtoHelper;
        encryptionRequestProtoHelper = keto::server_common::VectorUtils().copyStringToVector(
                blockWrapper.asn1_block());
        keto::key_store_utils::EncryptionResponseProtoHelper encryptionResponseProtoHelper(
                keto::server_common::fromEvent<keto::proto::EncryptResponse>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptRequest>(
                                keto::server_common::Events::ENCRYPT_ASN1::DECRYPT,encryptionRequestProtoHelper))));
        blockWrapper.set_asn1_block(encryptionResponseProtoHelper);
    }

    keto::block_db::SignedBlockWrapperProtoHelper signedBlockWrapperProtoHelper(blockWrapper);

    std::string nestedBlockStr;
    status = nestedTransaction->Get(readOptions,keyHelper,&nestedBlockStr);
    if (rocksdb::Status::OK() == status && rocksdb::Status::NotFound() != status) {
        keto::proto::NestedBlockMeta nestedBlockMeta;
        nestedBlockMeta.ParseFromString(nestedBlockStr);
        for (int index = 0; index < nestedBlockMeta.nested_hashs_size(); index++) {
            signedBlockWrapperProtoHelper.addNestedBlocks(
                    getBlock(
                            nestedBlockMeta.nested_hashs(index),resource));
        }
    }

    return signedBlockWrapperProtoHelper;
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

BlockChainPtr BlockChain::getChildByTransactionId(const keto::asn1::HashHelper& parentHash) {
    BlockChainPtr blockChainPtr = BlockChainCache::getInstance()->getBlockChain(parentHash);
    if (blockChainPtr) {
        return blockChainPtr;
    }
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* transactionTransaction = resource->getTransaction(Constants::TRANSACTIONS_INDEX);
    rocksdb::ReadOptions readOptions;
    std::string value;
    keto::rocks_db::SliceHelper parentHashSlice((const std::vector<uint8_t>)parentHash);
    auto status = transactionTransaction->Get(readOptions,parentHashSlice,&value);
    if (rocksdb::Status::OK() != status || rocksdb::Status::NotFound() == status) {
        BOOST_THROW_EXCEPTION(keto::block_db::InvalidParentHashIdentifierException());
    }
    keto::proto::TransactionMeta transactionMeta;
    transactionMeta.ParseFromString(value);
    return BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
                                        keto::server_common::VectorUtils().copyStringToVector(transactionMeta.block_chain_hash())));
}

void BlockChain::broadcastBlock(const keto::block_db::SignedBlockWrapperProtoHelper& signedBlockWrapperProtoHelper) {
    if (!masterChain) {
        return;
    }
    SignedBlockWrapperMessageProtoHelper signedBlockWrapperMessageProtoHelper(signedBlockWrapperProtoHelper);
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
            keto::server_common::Events::RPC_SERVER_BLOCK,signedBlockWrapperMessageProtoHelper));
}

}
}
