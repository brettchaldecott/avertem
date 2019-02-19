//
// Created by Brett Chaldecott on 2019/02/06.
//

#ifndef KETO_BLOCKCHAIN_HPP
#define KETO_BLOCKCHAIN_HPP

#include <string>
#include <memory>

#include "SignedBlock.h"

#include "keto/crypto/KeyLoader.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockChainMeta.hpp"
#include "keto/block_db/SignedBlockBuilder.hpp"
#include "keto/block_db/BlockChainCallback.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {

class BlockChain;
typedef std::shared_ptr<BlockChain> BlockChainPtr;

class BlockChain {
public:

    friend class BlockChainStore;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockChain(const BlockChain& orig) = delete;
    virtual ~BlockChain();

    bool requireGenesis();
    void writeBlock(const SignedBlockBuilderPtr& signedBlock, const BlockChainCallback& callback);
    keto::asn1::HashHelper getParentHash();
    keto::asn1::HashHelper getParentHash(const keto::asn1::HashHelper& transactionHash);
    BlockChainMetaPtr getBlockChainMeta();


private:
    bool inited;
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    BlockResourceManagerPtr blockResourceManagerPtr;
    BlockChainMetaPtr blockChainMetaPtr;
    BlockChainTangleMetaPtr activeTangle;
    std::vector<BlockChainPtr> sideChains;


    BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
               BlockResourceManagerPtr blockResourceManagerPtr);
    BlockChain(std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr,
               BlockResourceManagerPtr blockResourceManagerPtr,const std::vector<uint8_t>& id);

    keto::asn1::HashHelper selectParentHash();
    keto::asn1::HashHelper selectParentHashByLastBlockHash(const keto::asn1::HashHelper& id);

    void writeBlock(SignedBlock& signedBlock, const BlockChainCallback& callback);
    void load(const std::vector<uint8_t>& id);
    void persist();

    BlockChainPtr getChildPtr(const keto::asn1::HashHelper& parentHash);

};

}
}

#endif //KETO_BLOCKCHAIN_HPP