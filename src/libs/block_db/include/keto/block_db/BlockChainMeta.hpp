//
// Created by Brett Chaldecott on 2019/02/06.
//

#ifndef KETO_BLOCKCHAININFO_HPP
#define KETO_BLOCKCHAININFO_HPP

#include <string>
#include <memory>
#include <vector>
#include <map>

#include "BlockChainDB.pb.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockChainTangleMeta.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {

class BlockChainMeta;
typedef std::shared_ptr <BlockChainMeta> BlockChainMetaPtr;

class BlockChainMeta {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class BlockChain;
    friend class BlockChainTangleMeta;

    virtual ~BlockChainMeta();

    keto::asn1::HashHelper getHashId();
    std::time_t getCreated();
    void setCreated(const std::time_t& time);
    bool isEncrypted();
    void setEncrypted(bool encrypted);

    operator std::string() const;

    int tangleCount();
    BlockChainTangleMetaPtr selectTangleEntry();
    bool containsTangleInfo(const keto::asn1::HashHelper& id);
    BlockChainTangleMetaPtr getTangleEntry(int id);
    BlockChainTangleMetaPtr getTangleEntry(const keto::asn1::HashHelper& id);
    BlockChainTangleMetaPtr getTangleEntryByLastBlock(const keto::asn1::HashHelper& id);
    BlockChainTangleMetaPtr addTangle(const keto::asn1::HashHelper& hash);
private:
    keto::asn1::HashHelper hashId;
    std::time_t created;
    bool encrypted;
    BlockResourceManagerPtr blockResourceManagerPtr;
    std::vector<BlockChainTangleMetaPtr> tangles;
    std::map<std::vector<uint8_t>,BlockChainTangleMetaPtr> tangleMap;
    std::map<std::vector<uint8_t>,BlockChainTangleMetaPtr> tangleMapByLastBlock;

    BlockChainMeta(
            const std::vector<uint8_t>& id);
    BlockChainMeta(
            const keto::proto::BlockChainMeta& blockChainMeta);

    void updateTangleEntryByLastBlock(const keto::asn1::HashHelper& orig, const keto::asn1::HashHelper& update);
};

}
}


#endif //KETO_BLOCKCHAININFO_HPP
