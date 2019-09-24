//
// Created by Brett Chaldecott on 2019-09-19.
//

#ifndef KETO_BLOCKQUERYPROTOHELPER_HPP
#define KETO_BLOCKQUERYPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"


namespace keto {
namespace chain_query_common {

class BlockQueryProtoHelper;
typedef std::shared_ptr<BlockQueryProtoHelper> BlockQueryProtoHelperPtr;

class BlockQueryProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockQueryProtoHelper();
    BlockQueryProtoHelper(const keto::proto::BlockQuery& blockQuery);
    BlockQueryProtoHelper(const std::string& msg);
    BlockQueryProtoHelper(const BlockQueryProtoHelper& orig) = default;
    virtual ~BlockQueryProtoHelper();

    keto::asn1::HashHelper getBlockHashId() const;
    BlockQueryProtoHelper& setBlockHashId(const keto::asn1::HashHelper& hashHelper);

    int getNumberOfBlocks() const;
    BlockQueryProtoHelper& setNumberOfBlocks(int numberOfBlocks);

    operator keto::proto::BlockQuery() const;
    operator std::string() const;

private:
    keto::proto::BlockQuery blockQuery;
};


}
}


#endif //KETO_BLOCKQUERYPROTOHELPER_HPP
