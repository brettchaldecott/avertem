//
// Created by Brett Chaldecott on 2019-09-21.
//

#ifndef KETO_TRANSACTIONRESULTPROTOHELPER_HPP
#define KETO_TRANSACTIONRESULTPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/NumberHelper.hpp"

namespace keto {
namespace chain_query_common {

class TransactionResultProtoHelper;
typedef std::shared_ptr<TransactionResultProtoHelper> TransactionResultProtoHelperPtr;

class TransactionResultProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    TransactionResultProtoHelper();
    TransactionResultProtoHelper(const keto::proto::TransactionResult& transactionResult);
    TransactionResultProtoHelper(const std::string& msg);
    TransactionResultProtoHelper(const TransactionResultProtoHelper& orig) = default;
    virtual ~TransactionResultProtoHelper();

    keto::asn1::HashHelper getTransactionHashId();
    TransactionResultProtoHelper& setTransactionHashId(const keto::asn1::HashHelper& transactionHashId);

    std::time_t getCreated();
    TransactionResultProtoHelper& setCreated(const std::time_t& created);

    keto::asn1::HashHelper getParentTransactionHashId();
    TransactionResultProtoHelper& setParentTransactionHashId(const keto::asn1::HashHelper& transactionHashId);

    keto::asn1::HashHelper getSourceAccountHashId();
    TransactionResultProtoHelper& setSourceAccountHashId(const keto::asn1::HashHelper& sourceAccountHashId);

    keto::asn1::HashHelper getTargetAccountHashId();
    TransactionResultProtoHelper& setTargetAccountHashId(const keto::asn1::HashHelper& targetAccountHashId);

    keto::asn1::NumberHelper getValue();
    TransactionResultProtoHelper& setValue(const keto::asn1::NumberHelper& value);

    keto::asn1::SignatureHelper getSignature();
    TransactionResultProtoHelper& setSignature(const keto::asn1::SignatureHelper& signature);

    std::vector<keto::asn1::HashHelper> getChangesetHashes();
    TransactionResultProtoHelper& addChangesetHash(const keto::asn1::HashHelper& changeSetHash);

    std::vector<keto::asn1::HashHelper> getTransactionTraceHashes();
    TransactionResultProtoHelper& addTransactionTraceHash(const keto::asn1::HashHelper& transactionHash);

    int getStatus();
    TransactionResultProtoHelper& setStatus(int status);

    operator keto::proto::TransactionResult() const;
    operator std::string() const;
private:
    keto::proto::TransactionResult transactionResult;
};


}
}


#endif //KETO_TRANSACTIONRESULTPROTOHELPER_HPP
