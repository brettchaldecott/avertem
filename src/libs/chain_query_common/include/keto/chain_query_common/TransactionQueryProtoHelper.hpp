//
// Created by Brett Chaldecott on 2019-09-21.
//

#ifndef KETO_TRANSACTIONQUERYPROTOHELPER_HPP
#define KETO_TRANSACTIONQUERYPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

namespace keto {
namespace chain_query_common {

class TransactionQueryProtoHelper;
typedef std::shared_ptr<TransactionQueryProtoHelper> TransactionQueryProtoHelperPtr;

class TransactionQueryProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    TransactionQueryProtoHelper();
    TransactionQueryProtoHelper(const keto::proto::TransactionQuery& transactionQuery);
    TransactionQueryProtoHelper(const std::string& msg);
    TransactionQueryProtoHelper(const TransactionQueryProtoHelper& orig) = default;
    virtual ~TransactionQueryProtoHelper();

    keto::asn1::HashHelper getBlockHashId() const;
    TransactionQueryProtoHelper& setBlockHashId(const keto::asn1::HashHelper& blockHashId);

    keto::asn1::HashHelper getTransactionHashId() const;
    TransactionQueryProtoHelper& setTransactionHashId(const keto::asn1::HashHelper& transactionHashIds);

    keto::asn1::HashHelper getAccountHashId() const;
    TransactionQueryProtoHelper& setAccountHashId(const keto::asn1::HashHelper& accountHashIds);

    int getNumberOfTransactions() const;
    TransactionQueryProtoHelper& setNumberOfTransactions(int numberOfTransactions);

    operator keto::proto::TransactionQuery() const;
    operator std::string() const;
private:
    keto::proto::TransactionQuery transactionQuery;
};


}
}


#endif //KETO_TRANSACTIONQUERYPROTOHELPER_HPP
