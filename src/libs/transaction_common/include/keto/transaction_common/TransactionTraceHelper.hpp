//
// Created by Brett Chaldecott on 2019-09-24.
//

#ifndef KETO_TRANSACTIONTRACEHELPER_HPP
#define KETO_TRANSACTIONTRACEHELPER_HPP

#include <string>
#include <memory>
#include <vector>

#include "Transaction.h"
#include "SignedTransaction.h"
#include "SignedChangeSet.h"
#include "TransactionWrapper.h"
#include "TransactionTrace.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace transaction_common {

class TransactionTraceHelper;
typedef std::shared_ptr<TransactionTraceHelper> TransactionTraceHelperPtr;


class TransactionTraceHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    TransactionTraceHelper(TransactionTrace_t* transactionTrace);
    TransactionTraceHelper(const TransactionTraceHelper& orig) = delete;
    virtual ~TransactionTraceHelper();

    keto::asn1::HashHelper getTraceHash();
    keto::asn1::SignatureHelper getSignature();
    keto::asn1::HashHelper getSignatureHash();

private:
    TransactionTrace_t* transactionTrace;
};

}
}

#endif //KETO_TRANSACTIONTRACEHELPER_HPP
