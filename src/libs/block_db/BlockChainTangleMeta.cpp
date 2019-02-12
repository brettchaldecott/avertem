//
// Created by Brett Chaldecott on 2019/02/06.
//

#include "keto/block_db/BlockChainTangleMeta.hpp"
#include "keto/block_db/BlockChainMeta.hpp"

namespace keto {
    namespace block_db {


static std::string BlockChainTangleMetagetSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockChainTangleMeta::~BlockChainTangleMeta() {

}

keto::asn1::HashHelper BlockChainTangleMeta::getHash() {

}

keto::asn1::HashHelper BlockChainTangleMeta::getLastBlockHash() {

}

void BlockChainTangleMeta::setLastBlockHash(const keto::asn1::HashHelper& lastBlockHash) {

}

std::time_t BlockChainTangleMeta::getLastModified() {

}

void BlockChainTangleMeta::setLastModified(const std::time_t& lastModified) {

}


BlockChainTangleMeta::operator keto::proto::BlockChainTangleMeta() {

}

BlockChainTangleMeta::BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const keto::asn1::HashHelper& hash) {

}

BlockChainTangleMeta::BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const keto::proto::BlockChainTangleMeta& blockChainTangleMeta) {

}

BlockChainTangleMeta::BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const BlockChainTangleMeta& orig) {

}

}
}