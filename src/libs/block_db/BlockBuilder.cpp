/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockBuilder.cpp
 * Author: ubuntu
 * 
 * Created on March 13, 2018, 3:13 AM
 */

#include <vector>

#include "keto/common/MetaInfo.hpp"
#include "keto/block_db/BlockBuilder.hpp"
#include "keto/block_db/Exception.hpp"
#include "keto/block_db/MerkleUtils.hpp"
#include "keto/block_db/SignedChangeSetBuilder.hpp"
#include "keto/block_db/MerkleUtils.hpp"
#include "keto/block_db/BlockBuilder.hpp"
#include "keto/asn1/CloneHelper.hpp"


namespace keto {
namespace block_db {

std::string BlockBuilder::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

    
BlockBuilder::BlockBuilder() {
    this->block = (Block_t*)calloc(1, sizeof *block);
    this->block->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->block->date = keto::asn1::TimeHelper();
    
}


BlockBuilder::BlockBuilder(const keto::asn1::HashHelper& parentHash) {    
    this->block = (Block_t*)calloc(1, sizeof *block);
    this->block->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    this->block->date = keto::asn1::TimeHelper();
    this->block->parent = parentHash;
    
}
    

BlockBuilder::~BlockBuilder() {
    if (block) {
        ASN_STRUCT_FREE(asn_DEF_Block, block);
        block = NULL;
    }
}

BlockBuilder& BlockBuilder::addTransactionMessage(
        const keto::transaction_common::TransactionMessageHelperPtr transaction) {
    if (0 != ASN_SEQUENCE_ADD(&this->block->transactions,
            keto::asn1::clone<TransactionWrapper_t>(
            transaction->getTransactionWrapper()->operator TransactionWrapper_t*(),
            &asn_DEF_TransactionWrapper))) {
        BOOST_THROW_EXCEPTION(keto::block_db::FailedToAddTheTransactionException());
    }
    return (*this);
}

BlockBuilder& BlockBuilder::setAcceptedCheck(const SoftwareConsensus_t* softwareConsensus) {
    SoftwareConsensus_t* cloneSoftwareConsensus = keto::asn1::clone<SoftwareConsensus_t>(softwareConsensus,&asn_DEF_SoftwareConsensus);
    this->block->acceptedCheck = *cloneSoftwareConsensus;
    free(cloneSoftwareConsensus);
    return *this;
}

BlockBuilder& BlockBuilder::setValidateCheck(const SoftwareConsensus_t* softwareConsensus) {
    SoftwareConsensus_t* cloneSoftwareConsensus = keto::asn1::clone<SoftwareConsensus_t>(softwareConsensus,&asn_DEF_SoftwareConsensus);
    this->block->validateCheck = *cloneSoftwareConsensus;
    free(cloneSoftwareConsensus);
    return *this;
}

BlockBuilder::operator Block_t*() {
    MerkleUtils merkleUtils(this->block);
    
    block->merkelRoot = merkleUtils.computation();
    
    Block_t* result = block;
    block = 0;
    
    return result;
}

BlockBuilder::operator Block_t&() {
    MerkleUtils merkleUtils(this->block);
    
    block->merkelRoot = merkleUtils.computation();
    
    return *this->block;
}


}
}
