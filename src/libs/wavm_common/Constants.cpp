//
// Created by Brett Chaldecott on 2019/03/07.
//

#include "keto/wavm_common/Constants.hpp"

namespace keto {
namespace wavm_common {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

const char* Constants::REMOTE_SPARQL_QUERY = "REMOTE_SPARQL_QUERY";
const char* Constants::SESSION_SPARQL_QUERY = "SESSION_SPARQL_QUERY";

const char* Constants::SESSION_TYPES::HTTP = "http";
const char* Constants::SESSION_TYPES::TRANSACTION = "transaction";

const char* Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION = "base_account_transaction";
const char* Constants::SYSTEM_CONTRACT::FEE_PAYMENT = "fee_payment";
const char* Constants::SYSTEM_CONTRACT::NESTED_TRANSACTION = "nested_transaction";
const char* Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION = "faucet_transaction";


const std::vector<const char*> Constants::SYSTEM_CONTRACTS{
        Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FEE_PAYMENT,
        Constants::SYSTEM_CONTRACT::NESTED_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION
};

const char* Constants::TRANSACTION_BUILDER::MODEL::RDF = "RDF";

}
}

