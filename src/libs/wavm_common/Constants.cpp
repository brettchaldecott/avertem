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

}
}

