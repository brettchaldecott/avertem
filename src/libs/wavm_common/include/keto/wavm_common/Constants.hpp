//
// Created by Brett Chaldecott on 2019/03/07.
//

#ifndef KETO_WAVM_COMMON_CONSTANTS_HPP
#define KETO_WAVM_COMMON_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace wavm_common {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    static const char* REMOTE_SPARQL_QUERY;
    static const char* SESSION_SPARQL_QUERY;

    static const char* BLOCKCHAIN_CURRENCY_NAMESPACE;

    class SESSION_TYPES {
    public:
        static const char* HTTP;
        static const char* TRANSACTION;
    };

    class SYSTEM_CONTRACT {
    public:
        static const char* BASE_ACCOUNT_TRANSACTION;
        static const char* FEE_PAYMENT;
        static const char* NESTED_TRANSACTION;
        static const char* FAUCET_TRANSACTION;
        static const char* ACCOUNT_MANAGEMENT_TRANSACTION;
        static const char* SIDECHAIN_MANAGEMENT_TRANSACTION;
        static const char* NAMESPACE_MANAGEMENT_TRANSACTION;
    };
    static const std::vector<const char*> SYSTEM_CONTRACTS;
    static const std::vector<const char*> SYSTEM_NON_BALANCING_CONTRACTS;

    class TRANSACTION_BUILDER {
    public:
        class MODEL {
        public:
            static const char* RDF;
        };
    };
};


}
}


#endif //KETO_CONSTANTS_HPP
