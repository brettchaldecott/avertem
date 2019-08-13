//
// Created by Brett Chaldecott on 2019/02/06.
//

#include "keto/block_db/BlockChainTangleMeta.hpp"
#include "keto/block_db/BlockChainMeta.hpp"

namespace keto {
    namespace block_db {


std::string BlockChainTangleMeta::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockChainTangleMeta::~BlockChainTangleMeta() {

}

keto::asn1::HashHelper BlockChainTangleMeta::getHash() {
    return this->hash;
}

keto::asn1::HashHelper BlockChainTangleMeta::getLastBlockHash() {
    KETO_LOG_DEBUG << "[BlockChainTangleMeta::getLastBlockHash][" << this->hash.getHash(keto::common::StringEncoding::HEX)
                   << "] get the last hash for [" << lastBlockHash.getHash(keto::common::StringEncoding::HEX) << "]";
    return this->lastBlockHash;
}

void BlockChainTangleMeta::setLastBlockHash(const keto::asn1::HashHelper& lastBlockHash) {
    this->blockChainMeta->updateTangleEntryByLastBlock(this->lastBlockHash,lastBlockHash);
    KETO_LOG_DEBUG << "[BlockChainTangleMeta::setLastBlockHash][" << this->hash.getHash(keto::common::StringEncoding::HEX)
        << "] set the last hash for [" << lastBlockHash.getHash(keto::common::StringEncoding::HEX) << "]";
    this->lastBlockHash = lastBlockHash;
}

std::time_t BlockChainTangleMeta::getLastModified() {
    return this->lastModified;
}

void BlockChainTangleMeta::setLastModified(const std::time_t& lastModified) {
    this->lastModified = lastModified;
}


BlockChainTangleMeta::operator keto::proto::BlockChainTangleMeta() {
    keto::proto::BlockChainTangleMeta result;
    result.set_hash_id(this->hash);
    result.set_last_block_hash(this->lastBlockHash);
    google::protobuf::Timestamp timestamp;
    timestamp.set_seconds(this->lastModified);
    timestamp.set_nanos(0);
    *result.mutable_last_modified() = timestamp;
    return result;
}

BlockChainTangleMeta::BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const keto::asn1::HashHelper& hash) : blockChainMeta(blockChainMeta) {
    this->hash = hash;
    this->lastBlockHash = hash;
}

BlockChainTangleMeta::BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const keto::proto::BlockChainTangleMeta& blockChainTangleMeta) : blockChainMeta(blockChainMeta) {
    this->hash = blockChainTangleMeta.hash_id();
    this->lastBlockHash = blockChainTangleMeta.last_block_hash();
    this->lastModified = blockChainTangleMeta.last_modified().seconds();
}


}
}