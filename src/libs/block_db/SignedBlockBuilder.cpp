/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignedBlockBuilder.cpp
 * Author: ubuntu
 * 
 * Created on March 14, 2018, 2:51 PM
 */

#include "keto/block_db/SignedBlockBuilder.hpp"

#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/BerEncodingHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"
#include "keto/crypto/SignatureGenerator.hpp"
#include "keto/crypto/HashGenerator.hpp"

#include "keto/block_db/Exception.hpp"
#include "include/keto/block_db/SignedBlockBuilder.hpp"

namespace keto {
namespace block_db {

std::string SignedBlockBuilder::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignedBlockBuilder::SignedBlockBuilder() {
    this->signedBlock = (SignedBlock_t*)calloc(1, sizeof *signedBlock);
    this->signedBlock->date = keto::asn1::TimeHelper();
}

SignedBlockBuilder::SignedBlockBuilder(const BlockBuilderPtr& blockBuilderPtr) {
    Block_t* block = *blockBuilderPtr;
    this->signedBlock = (SignedBlock_t*)calloc(1, sizeof *signedBlock);
    this->signedBlock->date = keto::asn1::TimeHelper();
    this->signedBlock->block = *block;
    this->signedBlock->parent = keto::asn1::HashHelper(block->parent);
    this->signedBlock->hash = getBlockHash(block);
    free(block);
    for (BlockBuilderPtr nestedBlock : blockBuilderPtr->getNestedBlocks()) {
        this->nestedBlocks.push_back(SignedBlockBuilderPtr(new SignedBlockBuilder(nestedBlock)));
    }
}

SignedBlockBuilder::SignedBlockBuilder(const BlockBuilderPtr& blockBuilderPtr,
    const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) : 
        keyLoaderPtr(keyLoaderPtr) {
    Block_t* block = *blockBuilderPtr;
    this->signedBlock = (SignedBlock_t*)calloc(1, sizeof *signedBlock);
    this->signedBlock->date = keto::asn1::TimeHelper();
    this->signedBlock->block = *block;
    this->signedBlock->parent = keto::asn1::HashHelper(block->parent);
    this->signedBlock->hash = getBlockHash(block);
    free(block);
    for (BlockBuilderPtr nestedBlock : blockBuilderPtr->getNestedBlocks()) {
        this->nestedBlocks.push_back(SignedBlockBuilderPtr(new SignedBlockBuilder(nestedBlock,keyLoaderPtr)));
    }
}


SignedBlockBuilder::SignedBlockBuilder(const keto::proto::SignedBlockWrapper& signedBlockWrapper) {

}

SignedBlockBuilder::~SignedBlockBuilder() {
    this->nestedBlocks.clear();
    if (this->signedBlock) {
        ASN_STRUCT_FREE(asn_DEF_SignedBlock, signedBlock);
        signedBlock = NULL;
    }
}


SignedBlockBuilder& SignedBlockBuilder::sign() {
    if (!this->signedBlock) {
        BOOST_THROW_EXCEPTION(keto::block_db::SignedBlockReleasedException());
    }
    keto::crypto::SignatureGenerator generator(keyLoaderPtr);
    keto::asn1::HashHelper hashHelper(this->signedBlock->hash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    if (0 != ASN_SEQUENCE_ADD(&this->signedBlock->signatures,(Signature_t*)signatureHelper)) {
        BOOST_THROW_EXCEPTION(keto::block_db::FailedToAddTheTransactionException());
    }
    for (SignedBlockBuilderPtr signedBlockBuilderPtr: nestedBlocks) {
        signedBlockBuilderPtr->sign();
    }
    return (*this);
    
}

SignedBlockBuilder& SignedBlockBuilder::sign(std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) {
    if (!this->signedBlock) {
        BOOST_THROW_EXCEPTION(keto::block_db::SignedBlockReleasedException());
    }
    keto::crypto::SignatureGenerator generator(keyLoaderPtr);
    keto::asn1::HashHelper hashHelper(this->signedBlock->hash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    if (0 != ASN_SEQUENCE_ADD(&this->signedBlock->signatures,(Signature_t*)signatureHelper)) {
        BOOST_THROW_EXCEPTION(keto::block_db::FailedToAddTheTransactionException());
    }
    for (SignedBlockBuilderPtr signedBlockBuilderPtr: nestedBlocks) {
        signedBlockBuilderPtr->sign(keyLoaderPtr);
    }
    return (*this);

}

SignedBlockBuilder::operator SignedBlock_t*() {
    return this->signedBlock;
}

SignedBlockBuilder::operator SignedBlock_t&() {
    return *this->signedBlock;
}


std::vector<SignedBlockBuilderPtr> SignedBlockBuilder::getNestedBlocks() {
    return this->nestedBlocks;
}

keto::asn1::HashHelper SignedBlockBuilder::getParentHash() {
    return this->signedBlock->block.parent;
}

keto::asn1::HashHelper SignedBlockBuilder::getHash() {
    return this->signedBlock->hash;
}

keto::asn1::HashHelper SignedBlockBuilder::getFirstTransactionHash() {
    if(this->signedBlock->block.transactions.list.count) {
        return this->signedBlock->block.transactions.list.array[0]->transactionHash;
    }
    return getHash();
}

keto::asn1::HashHelper SignedBlockBuilder::getBlockHash(Block_t* block) {
    keto::asn1::SerializationHelper<Block_t> serializationHelper(block, &asn_DEF_Block);
    return keto::asn1::HashHelper(
        keto::crypto::HashGenerator().generateHash(
        serializationHelper.operator std::vector<uint8_t>&()));
}


}
}