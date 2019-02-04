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

BlockBuilder& BlockBuilder::setAcceptedCheck(SoftwareConsensus_t* softwareConsensus) {
    this->block->acceptedCheck = *softwareConsensus;
    free(softwareConsensus);
    return *this;
}

BlockBuilder& BlockBuilder::setValidateCheck(SoftwareConsensus_t* softwareConsensus) {
    this->block->validateCheck = *softwareConsensus;
    free(softwareConsensus);
    return *this;
}

BlockBuilder::operator Block_t*() {
    MerkleUtils merkleUtils(this->getCurrentHashs());
    
    block->merkelRoot = merkleUtils.computation();
    
    Block_t* result = block;
    block = 0;
    
    return result;
}

BlockBuilder::operator Block_t&() {
    MerkleUtils merkleUtils(getCurrentHashs());
    
    block->merkelRoot = merkleUtils.computation();
    
    return *this->block;
}


std::vector<keto::asn1::HashHelper> BlockBuilder::getCurrentHashs() {
    std::vector<keto::asn1::HashHelper> hashs;
    hashs.push_back(keto::asn1::HashHelper(this->block->parent));

    // add the transaction hashes
    for (int index = 0; index < this->block->transactions.list.count; index++) {
        TransactionWrapper* transactionWrapper = this->block->transactions.list.array[index];
        hashs.push_back(keto::asn1::HashHelper(transactionWrapper->transactionHash));
        for (int changeIndex = 0; changeIndex < transactionWrapper->changeSet.list.count; changeIndex++) {
            hashs.push_back(keto::asn1::HashHelper(transactionWrapper->changeSet.list.array[changeIndex]->changeSetHash));
        }
    }

    if (block->acceptedCheck.merkelRoot.size) {
        hashs.push_back(keto::asn1::HashHelper(this->block->acceptedCheck.merkelRoot));
    }
    if (block->validateCheck.merkelRoot.size) {
        hashs.push_back(keto::asn1::HashHelper(this->block->validateCheck.merkelRoot));
    }

    if (!hashs.size()) {
        BOOST_THROW_EXCEPTION(keto::block_db::ZeroLengthHashListException());
    }
    return hashs;
}

}
}
