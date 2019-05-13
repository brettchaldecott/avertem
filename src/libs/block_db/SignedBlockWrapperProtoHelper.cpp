//
// Created by Brett Chaldecott on 2019/04/23.
//

#include "keto/server_common/VectorUtils.hpp"

#include "keto/block_db/SignedBlockWrapperProtoHelper.hpp"

#include "keto/asn1/DeserializationHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"

namespace keto {
namespace block_db {

std::string SignedBlockWrapperProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignedBlockWrapperProtoHelper::SignedBlockWrapperProtoHelper(const keto::proto::BlockWrapper& blockWrapper) : signedBlock(NULL) {
    signedBlockWrapper.set_asn1_block_message(blockWrapper.asn1_block());
}

SignedBlockWrapperProtoHelper::SignedBlockWrapperProtoHelper(const keto::proto::SignedBlockWrapper& wrapper) :
        signedBlockWrapper(wrapper), signedBlock(NULL) {
}

SignedBlockWrapperProtoHelper::SignedBlockWrapperProtoHelper(const std::string& str) : signedBlock(NULL) {
    signedBlockWrapper.ParseFromString(str);
}

SignedBlockWrapperProtoHelper::SignedBlockWrapperProtoHelper(const keto::block_db::SignedBlockBuilderPtr& signedBlockBuilderPtr) : signedBlock(NULL) {
    populateSignedBlockWrapper(signedBlockWrapper,signedBlockBuilderPtr);
}

SignedBlockWrapperProtoHelper::~SignedBlockWrapperProtoHelper(){
    if (this->signedBlock) {
        ASN_STRUCT_FREE(asn_DEF_SignedBlock, signedBlock);
        signedBlock = NULL;
    }
}


keto::proto::SignedBlockWrapper SignedBlockWrapperProtoHelper::getSignedBlockWrapper() {
    return this->signedBlockWrapper;
}

SignedBlockWrapperProtoHelper::operator keto::proto::SignedBlockWrapper() {
    return this->signedBlockWrapper;
}

std::vector<SignedBlockWrapperProtoHelperPtr> SignedBlockWrapperProtoHelper::getNestedBlocks() {
    std::vector<SignedBlockWrapperProtoHelperPtr> result;
    for (int index = 0; index < this->signedBlockWrapper.nested_blocks_size(); index++) {
        result.push_back(SignedBlockWrapperProtoHelperPtr(
                new SignedBlockWrapperProtoHelper(this->signedBlockWrapper.nested_blocks(index))));
    }
    return result;
}

SignedBlockWrapperProtoHelper& SignedBlockWrapperProtoHelper::addNestedBlocks(
        const keto::proto::SignedBlockWrapper& wrapper) {
    *this->signedBlockWrapper.add_nested_blocks() = wrapper;
    return *this;
}

keto::asn1::HashHelper SignedBlockWrapperProtoHelper::getHash() {
    this->loadSignedBlock();
    return this->signedBlock->hash;
}

keto::asn1::HashHelper SignedBlockWrapperProtoHelper::getFirstTransactionHash() {
    this->loadSignedBlock();
    if(this->signedBlock->block.transactions.list.count) {
        return this->signedBlock->block.transactions.list.array[0]->transactionHash;
    }
    return getHash();
}

keto::asn1::HashHelper SignedBlockWrapperProtoHelper::getParentHash() {
    this->loadSignedBlock();
    return this->signedBlock->block.parent;
}


SignedBlockWrapperProtoHelper::operator SignedBlock_t&() {
    this->loadSignedBlock();

    return *this->signedBlock;
}

SignedBlockWrapperProtoHelper::operator SignedBlock_t*() {
    this->loadSignedBlock();

    return this->signedBlock;
}


SignedBlockWrapperProtoHelper::operator std::string() const {
    std::string signedBlockWrapperStr;
    this->signedBlockWrapper.SerializePartialToString(&signedBlockWrapperStr);
    return signedBlockWrapperStr;
}

SignedBlockWrapperProtoHelper& SignedBlockWrapperProtoHelper::operator = (const std::string& asn1Block) {
    this->signedBlockWrapper.set_asn1_block_message(asn1Block);
    return *this;
}


void SignedBlockWrapperProtoHelper::populateSignedBlockWrapper(keto::proto::SignedBlockWrapper& signedBlockWrapper,
                                                               const keto::block_db::SignedBlockBuilderPtr& signedBlockBuilderPtr) {
    std::vector<uint8_t> bytes = keto::asn1::SerializationHelper<SignedBlock_t>(
            (SignedBlock_t*)*signedBlockBuilderPtr,&asn_DEF_SignedBlock);
    signedBlockWrapper.set_asn1_block_message(keto::server_common::VectorUtils().copyVectorToString(
            bytes));
    for (keto::block_db::SignedBlockBuilderPtr nested : signedBlockBuilderPtr->getNestedBlocks()) {
        keto::proto::SignedBlockWrapper* nestedSignedBlockWrapper = signedBlockWrapper.add_nested_blocks();
        populateSignedBlockWrapper(*nestedSignedBlockWrapper,nested);
    }
}

void SignedBlockWrapperProtoHelper::loadSignedBlock() {
    if (!this->signedBlock) {
        this->signedBlock = keto::asn1::DeserializationHelper<SignedBlock_t>(
                (const uint8_t*)this->signedBlockWrapper.asn1_block_message().data(),
                this->signedBlockWrapper.asn1_block_message().size(),
                &asn_DEF_SignedBlock).takePtr();
    }
}

}
}
