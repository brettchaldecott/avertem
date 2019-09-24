//
// Created by Brett Chaldecott on 2019-09-20.
//

#include "keto/chain_query_common/BlockResultSetProtoHelper.hpp"

#include "keto/chain_query_common/Constants.hpp"

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace chain_query_common {

std::string BlockResultSetProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockResultSetProtoHelper::BlockResultSetProtoHelper() {
    this->resultSet.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

BlockResultSetProtoHelper::BlockResultSetProtoHelper(const keto::proto::BlockResultSet& resultSet) : resultSet(resultSet){
}

BlockResultSetProtoHelper::BlockResultSetProtoHelper(const std::string& msg) {
    this->resultSet.ParseFromString(msg);
}

BlockResultSetProtoHelper::~BlockResultSetProtoHelper() {

}

keto::asn1::HashHelper BlockResultSetProtoHelper::getStartBlockHashId() {
    return this->resultSet.start_block_hash_id();
}
BlockResultSetProtoHelper& BlockResultSetProtoHelper::setStartBlockHashId(const keto::asn1::HashHelper startBlockHashId) {
    this->resultSet.set_start_block_hash_id(startBlockHashId);
    return *this;
}

keto::asn1::HashHelper BlockResultSetProtoHelper::getEndBlockHashId() {
    return this->resultSet.end_block_hash_id();
}

BlockResultSetProtoHelper& BlockResultSetProtoHelper::setEndBlockHashId(const keto::asn1::HashHelper& hashHelper) {
    this->resultSet.set_end_block_hash_id(hashHelper);
    return *this;
}

int BlockResultSetProtoHelper::getNumberOfBlocks() {
    return this->resultSet.block_results_size();
}

std::vector<BlockResultProtoHelperPtr> BlockResultSetProtoHelper::getBlockResults() {
    std::vector<BlockResultProtoHelperPtr> result;
    for (int index = 0; index < this->resultSet.block_results_size(); index++) {
        result.push_back(BlockResultProtoHelperPtr(new BlockResultProtoHelper(this->resultSet.block_results(index))));
    }
    return result;
}

BlockResultSetProtoHelper& BlockResultSetProtoHelper::addBlockResult(const BlockResultProtoHelper& blockResultProtoHelper) {
    *this->resultSet.add_block_results() = blockResultProtoHelper;
    return *this;
}

BlockResultSetProtoHelper& BlockResultSetProtoHelper::addBlockResult(const BlockResultProtoHelperPtr& blockResultProtoHelperPtr) {
    *this->resultSet.add_block_results() = *blockResultProtoHelperPtr;
    return *this;
}

BlockResultSetProtoHelper::operator keto::proto::BlockResultSet() {
    return this->resultSet;
}

BlockResultSetProtoHelper::operator std::string() {
    return this->resultSet.SerializeAsString();
}


}
}