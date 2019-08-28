//
// Created by Brett Chaldecott on 2019/02/06.
//

#ifndef KETO_BLOCKCHAINTANGLEMETA_HPP
#define KETO_BLOCKCHAINTANGLEMETA_HPP

#include <string>
#include <memory>

#include "BlockChain.pb.h"
#include "BlockChainDB.pb.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/block_db/BlockResourceManager.hpp"
#include "keto/block_db/BlockChainTangleMeta.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {

class BlockChainMeta;
class BlockChainTangleMeta;
typedef std::shared_ptr<BlockChainTangleMeta> BlockChainTangleMetaPtr;


class BlockChainTangleMeta {
public:
    static std::string getHeaderVersion() {
            return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class BlockChainMeta;

    virtual ~BlockChainTangleMeta();

    keto::asn1::HashHelper getHash();
    keto::asn1::HashHelper getLastBlockHash();
    void setLastBlockHash(const keto::asn1::HashHelper& lastBlockHash);
    std::time_t getLastModified();
    void setLastModified(const std::time_t& lastModified);
    int incrementNumberOfAccounts();
    int getNumberOfAccounts();

    operator keto::proto::BlockChainTangleMeta();
private:
    BlockChainMeta* blockChainMeta;
    keto::asn1::HashHelper hash;
    keto::asn1::HashHelper lastBlockHash;
    std::time_t lastModified;
    int numberOfAccounts;

    BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const keto::asn1::HashHelper& hash);
    BlockChainTangleMeta(BlockChainMeta* blockChainMeta, const keto::proto::BlockChainTangleMeta& blockChainTangleMeta);
    BlockChainTangleMeta(const BlockChainTangleMeta& orig) = delete;

};


}
}


#endif //KETO_BLOCKCHAINTANGLEMETA_HPP
