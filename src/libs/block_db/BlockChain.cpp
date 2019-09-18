//
// Created by Brett Chaldecott on 2019/02/06.
//

#include <Block.h>
#include <keto/server_common/StringUtils.hpp>
#include <keto/server_common/Events.hpp>
#include "BlockChain.pb.h"

#include "keto/block_db/BlockChain.hpp"
#include "keto/block_db/Exception.hpp"
#include "keto/block_db/Constants.hpp"

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
#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/key_store_utils/EncryptionResponseProtoHelper.hpp"
#include "keto/key_store_utils/EncryptionRequestProtoHelper.hpp"

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


BlockChain::MasterTangleManager::MasterTangleManager(const BlockChainMetaPtr& blockChainMetaPtr) : blockChainMetaPtr(blockChainMetaPtr) {

}

BlockChain::MasterTangleManager::~MasterTangleManager() {

}

void BlockChain::MasterTangleManager::addTangle(const keto::asn1::HashHelper& tangle) {
    this->activeTangles[tangle] = this->blockChainMetaPtr->getTangleEntry(tangle);
}

std::vector<keto::asn1::HashHelper> BlockChain::MasterTangleManager::getActiveTangles() {
    std::vector<keto::asn1::HashHelper> keys;
    std::transform(
            this->activeTangles.begin(),
            this->activeTangles.end(),
            std::back_inserter(keys),
            [](const std::map<std::vector<uint8_t>,BlockChainTangleMetaPtr>::value_type
               &pair){return pair.first;});
    return keys;
}

void BlockChain::MasterTangleManager::setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    this->activeTangles.clear();
    for (keto::asn1::HashHelper tangle : tangles) {
        addTangle(tangle);
    }
}

//keto::asn1::HashHelper BlockChain::MasterTangleManager::getTangleHash() {
//    if (this->currentTangle) {
//        return this->currentTangle->getHash();
//    }
//    BOOST_THROW_EXCEPTION(keto::block_db::MasterTangleNotConfiguredException());
//}

keto::asn1::HashHelper BlockChain::MasterTangleManager::getParentHash() {
    if (this->currentTangle) {
        return this->currentTangle->getLastBlockHash();
    }
    BOOST_THROW_EXCEPTION(keto::block_db::MasterTangleNotConfiguredException("The required current tangle has not been configured"));
}

//keto::asn1::HashHelper BlockChain::MasterTangleManager::selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id) {
//    BlockChainTangleMetaPtr tangle =
//        this->blockChainMetaPtr->getTangleEntryByLastBlock(id);
//    if (!tangle) {
//        BOOST_THROW_EXCEPTION(keto::block_db::InvalidLastBlockHashException());
//    }
//    return tangle->getHash();
//}

keto::asn1::HashHelper BlockChain::MasterTangleManager::selectParentHash() {
    if (this->currentTangle) {
        return this->currentTangle->getLastBlockHash();
    }
    BOOST_THROW_EXCEPTION(keto::block_db::MasterTangleNotConfiguredException("The required current tangle has not been configured"));
}

void BlockChain::MasterTangleManager::setCurrentTangle(const keto::asn1::HashHelper& tangle) {
    this->currentTangle = this->activeTangles[tangle];
}

//
// The nested tangles
//
//
BlockChain::NestedTangleManager::NestedTangleManager(const BlockChainMetaPtr& blockChainMetaPtr) : blockChainMetaPtr(blockChainMetaPtr) {

}

BlockChain::NestedTangleManager::~NestedTangleManager() {

}

void BlockChain::NestedTangleManager::addTangle(const keto::asn1::HashHelper& tangle) {
    // do nothing
}

std::vector<keto::asn1::HashHelper> BlockChain::NestedTangleManager::getActiveTangles() {
    return std::vector<keto::asn1::HashHelper>();
}

void BlockChain::NestedTangleManager::setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) {

}

//keto::asn1::HashHelper BlockChain::NestedTangleManager::getTangleHash() {
//    if (activeTangle) {
//        return activeTangle->getLastBlockHash();
//    }
//    activeTangle = this->blockChainMetaPtr->selectTangleEntry();
//    if (!activeTangle) {
//        return keto::asn1::HashHelper();
//    }
//    return activeTangle->getHash();
//}

keto::asn1::HashHelper BlockChain::NestedTangleManager::getParentHash() {
    if (!this->activeTangle) {
        return selectParentHash();
    }
    return this->activeTangle->getLastBlockHash();
}

//keto::asn1::HashHelper BlockChain::NestedTangleManager::selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id) {
//    this->activeTangle =
//            this->blockChainMetaPtr->getTangleEntryByLastBlock(id);
//    if (!this->activeTangle) {
//        BOOST_THROW_EXCEPTION(keto::block_db::InvalidLastBlockHashException());
//    }
//    return this->activeTangle->getHash();
//}

keto::asn1::HashHelper BlockChain::NestedTangleManager::selectParentHash() {
    if (activeTangle) {
        return activeTangle->getLastBlockHash();
    }
    activeTangle = this->blockChainMetaPtr->selectTangleEntry();
    if (!activeTangle) {
        return keto::asn1::HashHelper();
    }
    return activeTangle->getLastBlockHash();
}


void BlockChain::NestedTangleManager::setCurrentTangle(const keto::asn1::HashHelper& tangle) {

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
    return this->tangleManagerInterfacePtr->getParentHash();
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

//keto::asn1::HashHelper BlockChain::selectParentHash() {
//    return this->tangleManagerInterfacePtr->selectParentHash();
//}

//keto::asn1::HashHelper BlockChain::selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id) {
//    return this->tangleManagerInterfacePtr->selectParentHashByLastBlockHash(id);
//}

//keto::asn1::HashHelper BlockChain::getTangleHash() {
//    return this->tangleManagerInterfacePtr->getTangleHash();
//}

bool BlockChain::writeBlock(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage, const BlockChainCallback& callback) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    SignedBlockWrapperMessageProtoHelper signedBlockWrapperMessageProtoHelper(signedBlockWrapperMessage);

    SignedBlockWrapperProtoHelperPtr signedBlockWrapperProtoHelperPtr =
            signedBlockWrapperMessageProtoHelper.getSignedBlockWrapper();

    // write the block and broadcast it if the block was written
    bool result = writeBlock(signedBlockWrapperProtoHelperPtr, callback);
    if (result) {
        // broadcast the block if we had to write it
        // if we did not the broadcast will not be proppergated
        broadcastBlock(*signedBlockWrapperProtoHelperPtr);
    }
    return result;
}

bool BlockChain::writeBlock(const SignedBlockWrapperProtoHelperPtr& signedBlockWrapperProtoHelperPtr, const BlockChainCallback& callback) {
    SignedBlock& signedBlock = *signedBlockWrapperProtoHelperPtr;

    BlockResourcePtr resource = blockResourceManagerPtr->getResource();

    // check for a duplicate block
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((const std::vector<uint8_t>)signedBlockWrapperProtoHelperPtr->getHash());
    std::string value;

    KETO_LOG_DEBUG << "[BlockChain::writeBlock] Look for the block : " << signedBlockWrapperProtoHelperPtr->getHash().getHash(keto::common::StringEncoding::HEX);
    auto status = blockTransaction->Get(readOptions,keyHelper,&value);

    if (rocksdb::Status::OK() == status && rocksdb::Status::NotFound() != status) {
        // ignore as the block already exists
        KETO_LOG_DEBUG << "[BlockChain::writeBlock] The block already exists in the store ignore it.";
        return false;
    }

    bool result = writeBlock(resource, signedBlock, callback);
    if (!result) {
        return result;
    }

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
        bool childResult = childPtr->writeBlock(signedNestedBlock,callback);
        if (result) {
            result = childResult;
        }
        *nestedBlockMeta.add_nested_hashs() = signedNestedBlock->getHash();
    }

    keto::asn1::HashHelper blockHash(signedBlock.hash);
    keto::rocks_db::SliceHelper blockHashHelper((const std::vector<uint8_t>)blockHash);

    std::string nestedBlockStr;
    nestedBlockMeta.SerializeToString(&nestedBlockStr);
    keto::rocks_db::SliceHelper nestedSliceHelper(nestedBlockStr);
    nestedTransaction->Put(blockHashHelper,nestedSliceHelper);

    persist();
    return result;
}

bool BlockChain::writeBlock(const SignedBlockBuilderPtr& signedBlockBuilderPtr, const BlockChainCallback& callback) {
    std::lock_guard<std::recursive_mutex> guard(this->classMutex);
    SignedBlock& signedBlock = *signedBlockBuilderPtr;

    BlockResourcePtr resource = blockResourceManagerPtr->getResource();

    if (!writeBlock(resource, signedBlock, callback)) {
        return false;
    }

    rocksdb::Transaction* nestedTransaction = resource->getTransaction(Constants::NESTED_INDEX);

    // handle the nested blocks
    keto::proto::NestedBlockMeta nestedBlockMeta;
    nestedBlockMeta.set_block_hash(blockChainMetaPtr->getHashId());
    bool result = true;
    for (SignedBlockBuilderPtr nestedBlock: signedBlockBuilderPtr->getNestedBlocks()) {
        BlockChainPtr nestedPtr;
        if (nestedBlock->getParentHash() == keto::block_db::Constants::GENESIS_HASH) {
            // create a new block chain using the transaction hash
            nestedPtr = BlockChainPtr(new BlockChain(this->dbManagerPtr,this->blockResourceManagerPtr,
                                                    nestedBlock->getFirstTransactionHash()));
        }  else {
            nestedPtr = getChildPtr(nestedBlock->getParentHash());
        }
        if (!nestedPtr->writeBlock(nestedBlock,callback)) {
            result = false;
        }
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
    return result;
}


bool BlockChain::writeBlock(BlockResourcePtr resource, SignedBlock& signedBlock, const BlockChainCallback& callback) {

    KETO_LOG_DEBUG << "[BlockChain::writeBlock]Write a new block";
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::Transaction* childTransaction = resource->getTransaction(Constants::CHILD_INDEX);
    rocksdb::Transaction* transactionTransaction = resource->getTransaction(Constants::TRANSACTIONS_INDEX);
    rocksdb::Transaction* accountTransaction = resource->getTransaction(Constants::ACCOUNTS_INDEX);

    keto::asn1::HashHelper blockHash(signedBlock.hash);

    KETO_LOG_DEBUG << "[BlockChain::writeBlock] Look for the block : " << blockHash.getHash(keto::common::StringEncoding::HEX);
    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((const std::vector<uint8_t>)blockHash);
    std::string value;
    auto blockStatus = blockTransaction->Get(readOptions,keyHelper,&value);

    if (rocksdb::Status::OK() == blockStatus && rocksdb::Status::NotFound() != blockStatus) {
        // ignore as the block already exists
        KETO_LOG_DEBUG << "[BlockChain::writeBlock] The block already exists in the store ignore it.";
        return false;
    }

    std::shared_ptr<keto::asn1::SerializationHelper<SignedBlock>> serializationHelperPtr =
            std::make_shared<keto::asn1::SerializationHelper<SignedBlock>>(
                    &signedBlock,&asn_DEF_SignedBlock);
    keto::rocks_db::SliceHelper blockHashHelper((const std::vector<uint8_t>)blockHash);
    keto::asn1::HashHelper parentHash(signedBlock.parent);
    keto::rocks_db::SliceHelper parentHashHelper((const std::vector<uint8_t>)parentHash);

    callback.prePersistBlock(blockChainMetaPtr->getHashId(),signedBlock);

    // check if there is a tangle for this entry
    BlockChainTangleMetaPtr blockChainTangleMetaPtr =
            this->blockChainMetaPtr->getTangleEntryByLastBlock(parentHash);
    if (!blockChainTangleMetaPtr) {
        KETO_LOG_INFO << "[BlockChain::writeBlock] parent hash is [" << parentHash.getHash(keto::common::StringEncoding::HEX)
                       << "] Add a new block chain tangle : " << blockHash.getHash(keto::common::StringEncoding::HEX);
        blockChainTangleMetaPtr = this->blockChainMetaPtr->addTangle(blockHash);
        this->tangleManagerInterfacePtr->addTangle(blockChainTangleMetaPtr->getHash());
        this->tangleManagerInterfacePtr->setCurrentTangle(blockChainTangleMetaPtr->getHash());
    }


    // setup the transaction indexing for the block
    for (int index = 0; index < signedBlock.block.transactions.list.count; index++) {
        TransactionWrapper_t* transactionWrapper = signedBlock.block.transactions.list.array[index];
        keto::transaction_common::TransactionWrapperHelper transactionWrapperHelper(transactionWrapper,false);

        if (!inited && index == 0) {
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
        std::string transactionValue;
        transactionMeta.SerializeToString(&transactionValue);
        keto::rocks_db::SliceHelper transactionMetaHelper(transactionValue);

        transactionTransaction->Put(keto::rocks_db::SliceHelper(
                (const std::vector<uint8_t>)transactionHash),transactionMetaHelper);

        // post persist
        callback.postPersistTransaction(blockChainMetaPtr->getHashId(),
                                       signedBlock, *transactionWrapper);

        // update the accounting information
        if (!accountExists(transactionWrapperHelper.getCurrentAccount())) {
            blockChainTangleMetaPtr->incrementNumberOfAccounts();
        }
        keto::proto::AccountMeta accountMeta;
        accountMeta.set_account_hash(transactionWrapperHelper.getCurrentAccount());
        accountMeta.set_block_tangle_hash_id(blockChainTangleMetaPtr->getHash());
        accountMeta.set_block_chain_hash(this->blockChainMetaPtr->getHashId());
        std::string accountValue;
        accountMeta.SerializeToString(&accountValue);
        KETO_LOG_INFO << "[BlockChain::writeBlock] for account [" << transactionWrapperHelper.getCurrentAccount().getHash(keto::common::StringEncoding::HEX)
                      << "] set the hash id to [" << blockChainTangleMetaPtr->getHash().getHash(keto::common::StringEncoding::HEX) << "] data size [" <<
                      accountValue.size() << "]";
        keto::rocks_db::SliceHelper accountMetaHelper(accountValue);

        accountTransaction->Put(keto::rocks_db::SliceHelper(
                (const std::vector<uint8_t>)transactionWrapperHelper.getCurrentAccount()),accountMetaHelper);
    }

    keto::proto::BlockWrapper blockWrapper;

    std::vector<uint8_t> blockBytes = (const std::vector<uint8_t>)(*serializationHelperPtr);

    keto::proto::BlockMeta blockMeta;
    blockMeta.set_block_hash((const std::string&)blockHash);
    blockMeta.set_block_parent_hash((const std::string&)parentHash);
    blockMeta.set_chain_id((const std::string&)this->blockChainMetaPtr->getHashId());
    blockMeta.set_encrypted(this->blockChainMetaPtr->isEncrypted());

    if (this->blockChainMetaPtr->isEncrypted()) {
        KETO_LOG_DEBUG << "[BlockChain::writeBlock] Decrypt the block";
        keto::key_store_utils::EncryptionRequestProtoHelper encryptionRequestProtoHelper;
        encryptionRequestProtoHelper = blockBytes;
        keto::key_store_utils::EncryptionResponseProtoHelper encryptionResponseProtoHelper(
                keto::server_common::fromEvent<keto::proto::EncryptResponse>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptRequest>(
                        keto::server_common::Events::ENCRYPT_ASN1::ENCRYPT,encryptionRequestProtoHelper))));
        blockWrapper.set_asn1_block(encryptionResponseProtoHelper);
        KETO_LOG_DEBUG << "[BlockChain::writeBlock] Decrypted the block : ";
    } else {
        KETO_LOG_DEBUG << "[BlockChain::writeBlock] Block is not encrypted copy bytes";
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
    auto childStatus = childTransaction->Get(readOptions, parentHashHelper, &childBlocks);
    keto::proto::BlockChildren blockChildren;
    if (rocksdb::Status::OK() == childStatus && rocksdb::Status::NotFound() != childStatus) {
        keto::proto::BlockChildren blockChildren;
        blockChildren.ParseFromString(childBlocks);
    }
    blockChildren.add_hashes((std::string)blockHash);
    std::string blockChildrenStr;
    blockChildren.SerializeToString(&blockChildrenStr);
    keto::rocks_db::SliceHelper blockChildrenHelper(blockChildrenStr);
    KETO_LOG_DEBUG << "[BlockChain::writeBlock] Add children to parent block : " << parentHash.getHash(keto::common::StringEncoding::HEX);
    childTransaction->Put(parentHashHelper,blockChildrenHelper);

    // update the tangle information
    KETO_LOG_DEBUG << "[BlockChain::writeBlock] Set the last block hash : "  << blockHash.getHash(keto::common::StringEncoding::HEX);
    blockChainTangleMetaPtr->setLastBlockHash(blockHash);
    blockChainTangleMetaPtr->setLastModified(keto::asn1::TimeHelper(signedBlock.date));

    // update the key tracking the parent key
    keto::rocks_db::SliceHelper parentKeyHelper((std::string(Constants::PARENT_KEY)));
    blockTransaction->Put(parentKeyHelper,blockHashHelper);

    callback.postPersistBlock(blockChainMetaPtr->getHashId(),signedBlock);

    KETO_LOG_DEBUG << "[BlockChain::writeBlock] Completed writing the block : " << blockHash.getHash(keto::common::StringEncoding::HEX);
    return true;
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

    //rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);


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
    KETO_LOG_DEBUG << "[BlockChain::processBlockSyncResponse] process the block : " << signedBlockBatch.blocks_size();
    for (int index = 0; index < signedBlockBatch.blocks_size(); index++) {
        bool write = this->writeBlock(signedBlockBatch.blocks(index),callback);
        if (complete) {
            complete = !write;
        }
    }
    KETO_LOG_DEBUG << "[BlockChain::processBlockSyncResponse] process the tangle block : " << signedBlockBatch.tangle_batches_size();
    for (int index = 0; index < signedBlockBatch.tangle_batches_size(); index++) {
        if (!processBlockSyncResponse(signedBlockBatch.tangle_batches(index),callback) && complete) {
            complete = false;
        }
    }
    KETO_LOG_DEBUG << "[BlockChain::processBlockSyncResponse] complete : " << complete;
    return complete;
}


keto::proto::SignedBlockBatch BlockChain::getBlockBatch(keto::asn1::HashHelper hash, BlockResourcePtr resource) {
    keto::proto::SignedBlockBatch signedBlockBatch;
    signedBlockBatch.set_start_block_id((std::string)hash);
    rocksdb::Transaction* childTransaction = resource->getTransaction(Constants::CHILD_INDEX);

    bool found = false;
    keto::asn1::HashHelper blockHash = hash;
    do {
        KETO_LOG_DEBUG<< "[BlockChain::getBlockBatch]" << " get the block : " << blockHash.getHash(keto::common::StringEncoding::HEX);
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
        KETO_LOG_DEBUG << "[BlockChain::getBlockBatch]" << " get the children for [" << blockHash.getHash(keto::common::StringEncoding::HEX) << "] : " << status.ToString();
        if (rocksdb::Status::OK() == status && rocksdb::Status::NotFound() != status) {
            keto::proto::BlockChildren blockChildren;
            blockChildren.ParseFromString(value);
            KETO_LOG_DEBUG << "[BlockChain::getBlockBatch]" << " loop through tangles attached to the block [" << blockHash.getHash(keto::common::StringEncoding::HEX) << "]";
            for (int index = 1; index < blockChildren.hashes_size(); index++) {
                keto::asn1::HashHelper childHash = blockChildren.hashes(index);
                KETO_LOG_DEBUG << "[BlockChain::getBlockBatch]" << " get tangle block hash [" << childHash.getHash(keto::common::StringEncoding::HEX) << "]";
                *signedBlockBatch.add_tangle_batches() = getBlockBatch(childHash,resource);
            }
            blockHash = keto::asn1::HashHelper(blockChildren.hashes(0));
            signedBlockBatch.set_end_block_id(blockHash);
            found = true;
            KETO_LOG_DEBUG << "[BlockChain::getBlockBatch]" << " The child hash is [" << blockHash.getHash(keto::common::StringEncoding::HEX) << "]";
        }
    } while(found);
    KETO_LOG_DEBUG << "[BlockChain::getBlockBatch]" << " return the block batch [" << hash.getHash(keto::common::StringEncoding::HEX) << "] : "
        << signedBlockBatch.blocks_size();
    return signedBlockBatch;
}

keto::proto::SignedBlockWrapper BlockChain::getBlock(keto::asn1::HashHelper hash, BlockResourcePtr resource) {

    KETO_LOG_DEBUG << "[BlockChain::getBlock]" << " Get the block : " << hash.getHash(keto::common::StringEncoding::HEX);
    rocksdb::Transaction* blockTransaction = resource->getTransaction(Constants::BLOCKS_INDEX);
    rocksdb::Transaction* nestedTransaction = resource->getTransaction(Constants::NESTED_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((std::vector<uint8_t>)hash);
    std::string value;

    auto status = blockTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() == status) {
        // not found
        std::stringstream ss;
        ss << "The last hash was not found in the store [" << hash.getHash(keto::common::StringEncoding::HEX) << "][" << status.ToString()
            << "] : " << status.code();
        BOOST_THROW_EXCEPTION(keto::block_db::InvalidLastBlockHashException(ss.str()));
    }

    keto::proto::BlockWrapper blockWrapper;
    blockWrapper.ParseFromString(value);

    // decrypt the block
    if (this->blockChainMetaPtr->isEncrypted()) {
        KETO_LOG_DEBUG << "[BlockChain::getBlock]" << " The block is encrypted and needs to be decrypted : " <<
            hash.getHash(keto::common::StringEncoding::HEX);
        keto::key_store_utils::EncryptionRequestProtoHelper encryptionRequestProtoHelper;
        encryptionRequestProtoHelper = keto::server_common::VectorUtils().copyStringToVector(
                blockWrapper.asn1_block());
        keto::key_store_utils::EncryptionResponseProtoHelper encryptionResponseProtoHelper(
                keto::server_common::fromEvent<keto::proto::EncryptResponse>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptRequest>(
                                keto::server_common::Events::ENCRYPT_ASN1::DECRYPT,encryptionRequestProtoHelper))));
        blockWrapper.set_asn1_block(encryptionResponseProtoHelper);
        KETO_LOG_DEBUG << "[BlockChain::getBlock]" << " Decrypted the buffer";
    }

    keto::block_db::SignedBlockWrapperProtoHelper signedBlockWrapperProtoHelper(blockWrapper);
    signedBlockWrapperProtoHelper.loadSignedBlock();

    std::string nestedBlockStr;
    status = nestedTransaction->Get(readOptions,keyHelper,&nestedBlockStr);
    if (rocksdb::Status::OK() == status && rocksdb::Status::NotFound() != status) {
        keto::proto::NestedBlockMeta nestedBlockMeta;
        nestedBlockMeta.ParseFromString(nestedBlockStr);
        KETO_LOG_DEBUG << "[BlockChain::getBlock]" << " The nexted hash blocks : " << nestedBlockMeta.nested_hashs_size();
        for (int index = 0; index < nestedBlockMeta.nested_hashs_size(); index++) {
            signedBlockWrapperProtoHelper.addNestedBlocks(
                    getBlock(
                            nestedBlockMeta.nested_hashs(index),resource));
        }
    }
    KETO_LOG_DEBUG << "[BlockChain::getBlock]" << " Return the signed block wrappers : " << hash.getHash(keto::common::StringEncoding::HEX);
    return signedBlockWrapperProtoHelper;
}


keto::proto::AccountChainTangle BlockChain::getAccountBlockTangle(const keto::proto::AccountChainTangle& accountChainTangle) {
    keto::proto::AccountChainTangle result = accountChainTangle;
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* accountTransaction =  resource->getTransaction(Constants::ACCOUNTS_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::asn1::HashHelper accountHash(accountChainTangle.account_id());
    keto::rocks_db::SliceHelper keyHelper((std::vector<uint8_t>)accountHash);
    std::string value;
    auto status = accountTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() == status) {
        KETO_LOG_INFO << "[BlockChain::getAccountBlockTangle] Could not find the account [" << accountHash.getHash(keto::common::StringEncoding::HEX)
            << "][" << status.ToString() << "][" << accountTransaction->GetNumKeys() << "]";
        result.set_found(false);
    } else {
        keto::proto::AccountMeta accountMeta;
        accountMeta.ParseFromString(value);
        result.set_found(true);
        result.set_chain_tangle_id(accountMeta.block_tangle_hash_id());
        keto::asn1::HashHelper tangleHashHelper(accountMeta.block_tangle_hash_id());
        KETO_LOG_INFO << "[BlockChain::getAccountBlockTangle] Find the account [" << accountHash.getHash(keto::common::StringEncoding::HEX)
                      << "][" << status.ToString() << "][" << tangleHashHelper.getHash(keto::common::StringEncoding::HEX) << "]";

    }

    return result;
}


bool BlockChain::getAccountTangle(const keto::asn1::HashHelper& accountHash, keto::asn1::HashHelper& tangleHash) {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* accountTransaction =  resource->getTransaction(Constants::ACCOUNTS_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((std::vector<uint8_t>)accountHash);
    std::string value;
    auto status = accountTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() == status) {
        return false;
    } else {
        keto::proto::AccountMeta accountMeta;
        accountMeta.ParseFromString(value);
        tangleHash = accountMeta.block_tangle_hash_id();
        return true;
    }
}

BlockChainTangleMetaPtr BlockChain::getTangleInfo(const keto::asn1::HashHelper& tangleHash) {
    return this->blockChainMetaPtr->getTangleEntry(tangleHash);
}

bool BlockChain::containsTangleInfo(const keto::asn1::HashHelper& tangleHash) {
    return this->blockChainMetaPtr->containsTangleInfo(tangleHash);
}

std::vector<keto::asn1::HashHelper> BlockChain::getActiveTangles() {
    return this->tangleManagerInterfacePtr->getActiveTangles();
}

keto::asn1::HashHelper BlockChain::getGrowTangle() {
    return this->tangleManagerInterfacePtr->getActiveTangles().back();
}

void BlockChain::setActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    this->tangleManagerInterfacePtr->setActiveTangles(tangles);
}

void BlockChain::setCurrentTangle(const keto::asn1::HashHelper& tangle) {
    this->tangleManagerInterfacePtr->setCurrentTangle(tangle);
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

    if (this->masterChain) {
        this->tangleManagerInterfacePtr = TangleManagerInterfacePtr(new BlockChain::MasterTangleManager(this->blockChainMetaPtr));
    } else {
        this->tangleManagerInterfacePtr = TangleManagerInterfacePtr(new BlockChain::NestedTangleManager(this->blockChainMetaPtr));
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
    KETO_LOG_DEBUG << "[BlockChain::broadcastBlock] push block via the server : " <<
        signedBlockWrapperMessageProtoHelper.getSignedBlockWrapper()->getHash().getHash(keto::common::StringEncoding::HEX);
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
            keto::server_common::Events::RPC_SERVER_BLOCK,signedBlockWrapperMessageProtoHelper));
    KETO_LOG_DEBUG << "[BlockChain::broadcastBlock] push block via the client : " <<
                   signedBlockWrapperMessageProtoHelper.getSignedBlockWrapper()->getHash().getHash(keto::common::StringEncoding::HEX);
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::SignedBlockWrapperMessage>(
            keto::server_common::Events::RPC_CLIENT_BLOCK,signedBlockWrapperMessageProtoHelper));
}


bool BlockChain::accountExists(const keto::asn1::HashHelper& accountHash) {
    BlockResourcePtr resource = blockResourceManagerPtr->getResource();
    rocksdb::Transaction* accountTransaction =  resource->getTransaction(Constants::ACCOUNTS_INDEX);

    rocksdb::ReadOptions readOptions;
    keto::rocks_db::SliceHelper keyHelper((std::vector<uint8_t>)accountHash);
    std::string value;
    auto status = accountTransaction->Get(readOptions,keyHelper,&value);
    if (rocksdb::Status::OK() != status && rocksdb::Status::NotFound() == status) {
        return false;
    }
    return true;
}



    }
}
