//
// Created by Brett Chaldecott on 2019-09-21.
//

#ifndef KETO_TRANSACTIONRESULTSETPROTOHELPER_HPP
#define KETO_TRANSACTIONRESULTSETPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/NumberHelper.hpp"

#include "keto/chain_query_common/TransactionResultProtoHelper.hpp"

namespace keto {
namespace chain_query_common {

class TransactionResultSetProtoHelper;
typedef std::shared_ptr<TransactionResultSetProtoHelper> TransactionResultSetProtoHelperPtr;

class TransactionResultSetProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    TransactionResultSetProtoHelper();
    TransactionResultSetProtoHelper(const keto::proto::TransactionResultSet& transactionResult);
    TransactionResultSetProtoHelper(const std::string& msg);
    TransactionResultSetProtoHelper(const TransactionResultSetProtoHelper& orig) = default;
    virtual ~TransactionResultSetProtoHelper();

    std::vector<TransactionResultProtoHelperPtr> getTransactions();
    TransactionResultSetProtoHelper& addTransaction(const TransactionResultProtoHelper& transactionResultProtoHelper);
    TransactionResultSetProtoHelper& addTransaction(const TransactionResultProtoHelperPtr& transactionResultProtoHelperPtr);

    int getNumberOfTransactions();

    operator keto::proto::TransactionResultSet();
    operator std::string();

private:
    keto::proto::TransactionResultSet transactionResultSet;

};


}
}


#endif //KETO_TRANSACTIONRESULTSETPROTOHELPER_HPP
