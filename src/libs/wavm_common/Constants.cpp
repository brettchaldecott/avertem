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

const char* Constants::BLOCKCHAIN_CURRENCY_NAMESPACE = "http://keto-coin.io/schema/rdf/1.0/keto/AccountTransaction#AccountTransaction";

const char* Constants::SESSION_TYPES::HTTP = "http";
const char* Constants::SESSION_TYPES::TRANSACTION = "transaction";

const char* Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION = "avertem__base_account_transaction";
const char* Constants::SYSTEM_CONTRACT::FEE_PAYMENT = "avertem__fee_payment";
const char* Constants::SYSTEM_CONTRACT::NESTED_TRANSACTION = "avertem__nested_transaction";
const char* Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION = "avertem__faucet_transaction";
const char* Constants::SYSTEM_CONTRACT::ACCOUNT_MANAGEMENT_TRANSACTION = "avertem__account_management_contract";
const char* Constants::SYSTEM_CONTRACT::SIDECHAIN_MANAGEMENT_TRANSACTION = "avertem__contract_sidechain_contract";
const char* Constants::SYSTEM_CONTRACT::NAMESPACE_MANAGEMENT_TRANSACTION = "avertem__contract_namespace_contract";


const std::vector<const char*> Constants::SYSTEM_CONTRACTS{
        Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FEE_PAYMENT,
        Constants::SYSTEM_CONTRACT::NESTED_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION,
        Constants::SYSTEM_CONTRACT::ACCOUNT_MANAGEMENT_TRANSACTION,
        Constants::SYSTEM_CONTRACT::SIDECHAIN_MANAGEMENT_TRANSACTION
};

const std::vector<const char*> Constants::SYSTEM_NON_BALANCING_CONTRACTS {
        Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FEE_PAYMENT,
        Constants::SYSTEM_CONTRACT::SIDECHAIN_MANAGEMENT_TRANSACTION
};

const char* Constants::TRANSACTION_BUILDER::MODEL::RDF = "RDF";

}
}

