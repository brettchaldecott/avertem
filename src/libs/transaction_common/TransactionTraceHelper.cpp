//
// Created by Brett Chaldecott on 2019-09-24.
//

#include "keto/transaction_common/TransactionTraceHelper.hpp"



namespace keto {
namespace transaction_common {

std::string TransactionTraceHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionTraceHelper::TransactionTraceHelper(TransactionTrace_t* transactionTrace) : transactionTrace(transactionTrace) {
}

TransactionTraceHelper::~TransactionTraceHelper() {
}

keto::asn1::HashHelper TransactionTraceHelper::getTraceHash() {
    return this->transactionTrace->traceHash;
}

keto::asn1::SignatureHelper TransactionTraceHelper::getSignature() {
    return this->transactionTrace->signature;
}

keto::asn1::HashHelper TransactionTraceHelper::getSignatureHash() {
    return this->transactionTrace->signatureHash;
}

}
}