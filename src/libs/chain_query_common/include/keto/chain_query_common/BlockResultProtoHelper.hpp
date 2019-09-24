//
// Created by Brett Chaldecott on 2019-09-20.
//

#ifndef KETO_BLOCKRESULTPROTOHELPER_HPP
#define KETO_BLOCKRESULTPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

#include "keto/chain_query_common/TransactionResultProtoHelper.hpp"

namespace keto {
namespace chain_query_common {

class BlockResultProtoHelper;
typedef std::shared_ptr<BlockResultProtoHelper> BlockResultProtoHelperPtr;


class BlockResultProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockResultProtoHelper();
    BlockResultProtoHelper(const keto::proto::BlockResult& blockResult);
    BlockResultProtoHelper(const std::string& msg);
    BlockResultProtoHelper(const BlockResultProtoHelper& orig) = default;
    virtual ~BlockResultProtoHelper();

    keto::asn1::HashHelper getTangleHashId();
    BlockResultProtoHelper& setTangleHashId(const keto::asn1::HashHelper& hashHelper);

    keto::asn1::HashHelper getBlockHashId();
    BlockResultProtoHelper& setBlockHashId(const keto::asn1::HashHelper& hashHelper);

    std::time_t getCreated();
    BlockResultProtoHelper& setCreated(const std::time_t& created);

    keto::asn1::HashHelper getParentBlockHashId();
    BlockResultProtoHelper& setParentBlockHashId(const keto::asn1::HashHelper& parentBlockHashId);

    std::vector<TransactionResultProtoHelperPtr> getTransactions();
    BlockResultProtoHelper& addTransaction(const TransactionResultProtoHelper& transactionResultProtoHelper);
    BlockResultProtoHelper& addTransaction(const TransactionResultProtoHelperPtr& transactionResultProtoHelperPtr);

    keto::asn1::HashHelper getMerkelRoot();
    BlockResultProtoHelper& setMerkelRoot(const keto::asn1::HashHelper& merkelRoot);

    keto::asn1::HashHelper getAcceptedHash();
    BlockResultProtoHelper& setAcceptedHash(const keto::asn1::HashHelper& acceptedHash);

    keto::asn1::HashHelper getValidationHash();
    BlockResultProtoHelper& setValidationHash(const keto::asn1::HashHelper& validationHash);

    keto::asn1::SignatureHelper getSignature();
    BlockResultProtoHelper& setSignature(const keto::asn1::SignatureHelper& validationHash);

    int getBlockHeight();
    BlockResultProtoHelper& setBlockHeight(int blockHeight);

    operator keto::proto::BlockResult() const;
    operator std::string() const;

private:
    keto::proto::BlockResult blockResult;
};

}
}


#endif //KETO_BLOCKRESULTPROTOHELPER_HPP
