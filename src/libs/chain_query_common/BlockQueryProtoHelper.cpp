//
// Created by Brett Chaldecott on 2019-09-19.
//

#include "keto/chain_query_common/BlockQueryProtoHelper.hpp"
#include "keto/chain_query_common/Constants.hpp"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace chain_query_common {


std::string BlockQueryProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockQueryProtoHelper::BlockQueryProtoHelper() {
    this->blockQuery.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    this->blockQuery.set_number_of_blocks(Constants::DEFAULT_NUMBER_OF_BLOCKS);
}

BlockQueryProtoHelper::BlockQueryProtoHelper(const keto::proto::BlockQuery& blockQuery) : blockQuery(blockQuery) {

}

BlockQueryProtoHelper::BlockQueryProtoHelper(const std::string& msg) {
    this->blockQuery.ParseFromString(msg);
}

BlockQueryProtoHelper::~BlockQueryProtoHelper() {

}

keto::asn1::HashHelper BlockQueryProtoHelper::getBlockHashId() const {
    return this->blockQuery.block_hash_id();
}

BlockQueryProtoHelper& BlockQueryProtoHelper::setBlockHashId(const keto::asn1::HashHelper& hashHelper) {
    this->blockQuery.set_block_hash_id(hashHelper);
    return *this;
}

int BlockQueryProtoHelper::getNumberOfBlocks() const {
    return this->blockQuery.number_of_blocks();
}

BlockQueryProtoHelper& BlockQueryProtoHelper::setNumberOfBlocks(int numberOfBlocks) {
    if (numberOfBlocks > Constants::MAX_NUMBER_OF_BLOCKS) {
        this->blockQuery.set_number_of_blocks(Constants::MAX_NUMBER_OF_BLOCKS);
    } else {
        this->blockQuery.set_number_of_blocks(numberOfBlocks);
    }
    return *this;
}

BlockQueryProtoHelper::operator keto::proto::BlockQuery() const {
    return this->blockQuery;
}

BlockQueryProtoHelper::operator std::string() const {
    return this->blockQuery.SerializeAsString();
}


}
}