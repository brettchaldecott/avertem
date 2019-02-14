//
// Created by Brett Chaldecott on 2019/02/06.
//

#include <chrono>

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
    blockMeta.set_hash_id(hashId);
    blockMeta.set_encrypted(this->encrypted);

    google::protobuf::Timestamp timestamp;
    timestamp.set_seconds(this->created);
    timestamp.set_nanos(0);
    *blockMeta.mutable_created_date() = timestamp;

    for (BlockChainTangleMetaPtr blockChainTangleMetaPtr: this->tangles) {
        *blockMeta.add_tangle() = *blockChainTangleMetaPtr;
    }
    std::string value;
    blockMeta.SerializeToString(&value);
    return value;
}

int BlockChainMeta::tangleCount() {
    return this->tangles.size();
}

BlockChainTangleMetaPtr BlockChainMeta::selectTangleEntry() {
    if (!this->tangles.size()) {
        return BlockChainTangleMetaPtr();
    }
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,this->tangles.size() -1);
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

BlockChainTangleMetaPtr BlockChainMeta::addTangle(const keto::asn1::HashHelper& hash) {
    BlockChainTangleMetaPtr blockChainTangleMetaPtr(new BlockChainTangleMeta(this,hash));
    this->tangles.push_back(blockChainTangleMetaPtr);
    this->tangleMap[blockChainTangleMetaPtr->getHash()] = blockChainTangleMetaPtr;
    this->tangleMapByLastBlock[blockChainTangleMetaPtr->getLastBlockHash()] = blockChainTangleMetaPtr;
    return blockChainTangleMetaPtr;
}

BlockChainMeta::BlockChainMeta(
        const std::vector<uint8_t>& id) {
    this->hashId = keto::asn1::HashHelper(id);
}

BlockChainMeta::BlockChainMeta(
        const keto::proto::BlockChainMeta& blockChainMeta) {
    this->hashId = keto::asn1::HashHelper(blockChainMeta.hash_id());
    this->encrypted = blockChainMeta.encrypted();
    this->created = blockChainMeta.created_date().seconds();
    for (int index = 0; index < blockChainMeta.tangle_size(); index++) {
        BlockChainTangleMetaPtr blockChainTangleMetaPtr(new BlockChainTangleMeta(this,blockChainMeta.tangle(index)));
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