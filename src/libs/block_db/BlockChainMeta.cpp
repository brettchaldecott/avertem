//
// Created by Brett Chaldecott on 2019/02/06.
//

#include "keto/block_db/BlockChainMeta.hpp"


namespace keto {
namespace block_db {

std::string BlockChainMeta::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockChainMeta::~BlockChainMeta() {

}

keto::asn1::HashHelper BlockChainMeta::getHashId() {
    return this->hashId;
}

std::time_t BlockChainMeta::getCreated() {
    return this->created;
}

void BlockChainMeta::setCreated(const std::time_t& time) {
    this->created = time;
}

bool BlockChainMeta::isEncrypted() {
    return this->encrypted;
}

void BlockChainMeta::setEncrypted(bool encrypted) {
    this->encrypted = encrypted;
}

BlockChainMeta::operator std::string() const {
    keto::proto::BlockChainMeta blockMeta;
    blockMeta.set_hash_id((const std::vector<uint8_t>)hashId);
    blockMeta.set_encrypted(this->encrypted);
    blockMeta.created_date.set_seconds(this->created);
    blockMeta.created_date.set_nanos(0);
    for (BlockChainTangleMetaPtr blockChainTangleMetaPtr: this->tangles) {
        keto::proto::BlockChainTangleMeta* blockChainTangleMeta = blockMeta.add_tangle();
        *blockChainTangleMeta = *blockChainTangleMetaPtr;
    }
    std::string value;
    blockMeta.SerializeToString(&value);
    return value;
}

int BlockChainMeta::tangleCount() {
    return this->tangles.size();
}

BlockChainTangleMetaPtr BlockChainMeta::selectTangleEntry() {
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,this->tangles.size());
    distribution(stdGenerator);
    return this->tangles[distribution(stdGenerator)];
}

BlockChainTangleMetaPtr BlockChainMeta::getTangleEntry(int id) {
    return this->tangles[id];
}

BlockChainTangleMetaPtr BlockChainMeta::getTangleEntry(const keto::asn1::HashHelper& id) {
    return this->tangleMap[id];
}

BlockChainTangleMetaPtr BlockChainMeta::getTangleEntryByLastBlock(const keto::asn1::HashHelper& id) {
    return this->tangleMapByLastBlock[id];
}

BlockChainTangleMetaPtr& BlockChainMeta::addTangle(const keto::asn1::HashHelper& hash) {
    BlockChainTangleMetaPtr blockChainTangleMetaPtr(new BlockChainTangleMeta(this,hash));
    this->tangles.push_back(blockChainTangleMetaPtr);
    this->tangleMap[blockChainTangleMetaPtr->getHash()] = blockChainTangleMetaPtr;
    this->tangleMapByLastBlock[blockChainTangleMetaPtr->getLastBlockHash()] = blockChainTangleMetaPtr;
    return blockResourceManagerPtr;
}

BlockChainMeta::BlockChainMeta(
        const std::vector<uint8_t>& id) {
    this->hashId = keto::asn1::HashHelper(id);
}

BlockChainMeta::BlockChainMeta(
        const keto::proto::BlockMeta& blockMeta) : {
    this->hashId = keto::asn1::HashHelper(blockMeta.hash_id());
    this->encrypted = blockMeta.encrypted();
    this->created = blockMeta.created_date.seconds();
    for (int index = 0; index < blockMeta.tangle_size(); index++) {
        BlockChainTangleMetaPtr blockChainTangleMetaPtr(new BlockChainTangleMeta(blockMeta.tangle(index)));
        this->tangles.push_back(blockChainTangleMetaPtr);
        this->tangleMap[blockChainTangleMetaPtr->getHash()] = blockChainTangleMetaPtr;
        this->tangleMapByLastBlock[blockChainTangleMetaPtr->getLastBlockHash()] = blockChainTangleMetaPtr;
    }
}


void BlockChainMeta::updateTangleEntryByLastBlock(const keto::asn1::HashHelper& orig,
        const keto::asn1::HashHelper& update) {
    this->tangleMapByLastBlock[update] = this->tangleMapByLastBlock[orig];
    this->tangleMapByLastBlock.erase(orig);
}

}
}