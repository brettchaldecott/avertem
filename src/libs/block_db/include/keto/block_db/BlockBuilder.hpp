/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockBuilder.hpp
 * Author: ubuntu
 *
 * Created on March 13, 2018, 3:13 AM
 */

#ifndef BLOCKBUILDER_HPP
#define BLOCKBUILDER_HPP

#include <string>
#include <vector>
#include <memory>

#include "SignedTransaction.h"
#include "SignedChangeSet.h"
#include "TransactionMessage.h"
#include "Block.h"
#include "SoftwareConsensus.h"

#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"



namespace keto {
namespace block_db {

class BlockBuilder;
typedef std::shared_ptr<BlockBuilder> BlockBuilderPtr;

class BlockBuilder {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class SignedBlockBuilder;

    BlockBuilder();
    BlockBuilder(const keto::asn1::HashHelper& parentHash);
    BlockBuilder(const BlockBuilder& orig) = delete;
    virtual ~BlockBuilder();



    BlockBuilder& addTransactionMessage(
            const keto::transaction_common::TransactionMessageHelperPtr transaction);
    BlockBuilder& setAcceptedCheck(SoftwareConsensus_t* softwareConsensus);
    BlockBuilder& setValidateCheck(SoftwareConsensus_t* softwareConsensus);

    std::vector<keto::asn1::HashHelper> getCurrentHashs();

    std::vector<BlockBuilderPtr> getNestedBlocks();

    bool matches(const keto::asn1::HashHelper& parentHash);

private:
    Block_t* block;
    keto::asn1::HashHelper parentHash;
    std::vector<BlockBuilderPtr> nestedBlocks;
    std::set<std::vector<std::uint8_t>> transactionIds;

    operator Block_t*();
    operator Block_t&();





};


}
}

#endif /* BLOCKBUILDER_HPP */

