/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignedBlockBuilder.hpp
 * Author: ubuntu
 *
 * Created on March 14, 2018, 2:51 PM
 */

#ifndef SIGNEDBLOCKBUILDER_HPP
#define SIGNEDBLOCKBUILDER_HPP

#include <string>
#include <memory>

#include "Block.h"
#include "SignedBlock.h"

#include "BlockChain.pb.h"

#include "keto/asn1/PrivateKeyHelper.hpp"
#include "keto/crypto/KeyLoader.hpp"
#include "keto/block_db/BlockBuilder.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {

class SignedBlockBuilder;
typedef std::shared_ptr<SignedBlockBuilder> SignedBlockBuilderPtr;

class SignedBlockBuilder {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class BlockChain;
    
    SignedBlockBuilder();
    SignedBlockBuilder(const BlockBuilderPtr& blockBuilderPtr);
    SignedBlockBuilder(const BlockBuilderPtr& blockBuilderPtr,
        const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr);
    SignedBlockBuilder(const keto::proto::SignedBlockWrapper& signedBlockWrapper);

    SignedBlockBuilder(const SignedBlockBuilder& orig) = delete;
    virtual ~SignedBlockBuilder();
    
    SignedBlockBuilder& sign();
    SignedBlockBuilder& sign(std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr);
    
    std::vector<SignedBlockBuilderPtr> getNestedBlocks();
    keto::asn1::HashHelper getHash();
    keto::asn1::HashHelper getFirstTransactionHash();
    keto::asn1::HashHelper getParentHash();

    operator SignedBlock_t*();
    operator SignedBlock_t&();

private:
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    SignedBlock_t* signedBlock;
    std::vector<SignedBlockBuilderPtr> nestedBlocks;
    
    keto::asn1::HashHelper getBlockHash(Block_t* block);
    SignedBlockBuilder& setBlock(Block_t* block);


};


}
}

#endif /* SIGNEDBLOCKBUILDER_HPP */

