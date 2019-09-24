//
// Created by Brett Chaldecott on 2019-09-20.
//

#ifndef KETO_BLOCKRESULTSETPROTOHELPER_HPP
#define KETO_BLOCKRESULTSETPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

#include "keto/chain_query_common/BlockResultProtoHelper.hpp"

namespace keto {
namespace chain_query_common {

class BlockResultSetProtoHelper;
typedef std::shared_ptr<BlockResultSetProtoHelper> BlockResultSetProtoHelperPtr;

class BlockResultSetProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockResultSetProtoHelper();
    BlockResultSetProtoHelper(const keto::proto::BlockResultSet& resultSet);
    BlockResultSetProtoHelper(const std::string& msg);
    BlockResultSetProtoHelper(const BlockResultSetProtoHelper& orig) = default;
    virtual ~BlockResultSetProtoHelper();

    keto::asn1::HashHelper getStartBlockHashId();
    BlockResultSetProtoHelper& setStartBlockHashId(const keto::asn1::HashHelper startBlockHashId);

    keto::asn1::HashHelper getEndBlockHashId();
    BlockResultSetProtoHelper& setEndBlockHashId(const keto::asn1::HashHelper& hashHelper);

    int getNumberOfBlocks();

    std::vector<BlockResultProtoHelperPtr> getBlockResults();
    BlockResultSetProtoHelper& addBlockResult(const BlockResultProtoHelper& blockResultProtoHelper);
    BlockResultSetProtoHelper& addBlockResult(const BlockResultProtoHelperPtr& blockResultProtoHelperPtr);

    operator keto::proto::BlockResultSet();
    operator std::string();

private:
    keto::proto::BlockResultSet resultSet;
};

}
}



#endif //KETO_BLOCKRESULTSETPROTOHELPER_HPP
