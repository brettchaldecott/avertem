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

SignedBlockBuilder::SignedBlockBuilder(const SignedBlock_t* signedBlock) {
    this->signedBlock = keto::asn1::clone<SignedBlock_t>(signedBlock,&asn_DEF_SignedBlock);
}

SignedBlockBuilder::SignedBlockBuilder(
    const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) : 
        keyLoaderPtr(keyLoaderPtr) {
    this->signedBlock = (SignedBlock_t*)calloc(1, sizeof *signedBlock);
    this->signedBlock->date = keto::asn1::TimeHelper();
}

SignedBlockBuilder::SignedBlockBuilder(Block_t* block,
    const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) : 
        keyLoaderPtr(keyLoaderPtr) {
    this->signedBlock = (SignedBlock_t*)calloc(1, sizeof *signedBlock);
    this->signedBlock->date = keto::asn1::TimeHelper();
    this->signedBlock->block = *block;
    this->signedBlock->parent = keto::asn1::HashHelper(block->parent);
    this->signedBlock->hash = getBlockHash(block);
    free(block);
}

SignedBlockBuilder::~SignedBlockBuilder() {
    if (this->signedBlock) {
        ASN_STRUCT_FREE(asn_DEF_SignedBlock, signedBlock);
        signedBlock = NULL;
    }
}


SignedBlockBuilder& SignedBlockBuilder::setPrivateKey(
    const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) {
    this->keyLoaderPtr = keyLoaderPtr;
    return (*this);
}

SignedBlockBuilder& SignedBlockBuilder::setBlock(Block_t* block) {
    if (!this->signedBlock) {
        BOOST_THROW_EXCEPTION(keto::block_db::SignedBlockReleasedException());
    }
    this->signedBlock->block = *block;
    this->signedBlock->parent = keto::asn1::HashHelper(block->parent);
    this->signedBlock->hash = getBlockHash(block);
    free(block);
    return (*this);
}

SignedBlockBuilder& SignedBlockBuilder::sign() {
    if (!this->signedBlock) {
        BOOST_THROW_EXCEPTION(keto::block_db::SignedBlockReleasedException());
    }
    keto::crypto::SignatureGenerator generator(*keyLoaderPtr);
    keto::asn1::HashHelper hashHelper(this->signedBlock->hash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    if (0 != ASN_SEQUENCE_ADD(&this->signedBlock->signatures,(Signature_t*)signatureHelper)) {
        BOOST_THROW_EXCEPTION(keto::block_db::FailedToAddTheTransactionException());
    }
    return (*this);
    
}

SignedBlockBuilder& SignedBlockBuilder::sign(std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr) {
    if (!this->signedBlock) {
        BOOST_THROW_EXCEPTION(keto::block_db::SignedBlockReleasedException());
    }
    keto::crypto::SignatureGenerator generator(*keyLoaderPtr);
    keto::asn1::HashHelper hashHelper(this->signedBlock->hash);
    keto::asn1::SignatureHelper signatureHelper(generator.sign(hashHelper));
    if (0 != ASN_SEQUENCE_ADD(&this->signedBlock->signatures,(Signature_t*)signatureHelper)) {
        BOOST_THROW_EXCEPTION(keto::block_db::FailedToAddTheTransactionException());
    }
    return (*this);

}

SignedBlockBuilder::operator SignedBlock_t*() {
    SignedBlock_t* result = this->signedBlock;
    this->signedBlock = 0;
    return result;
}

SignedBlockBuilder::operator SignedBlock_t&() {
    return *this->signedBlock;
}


keto::asn1::HashHelper SignedBlockBuilder::getBlockHash(Block_t* block) {
    keto::asn1::SerializationHelper<Block_t> serializationHelper(block, &asn_DEF_Block);
    return keto::asn1::HashHelper(
        keto::crypto::HashGenerator().generateHash(
        serializationHelper.operator std::vector<uint8_t>&()));
}


}
}