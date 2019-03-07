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

    class SESSION_TYPES {
    public:
        static const char* HTTP;
        static const char* TRANSACTION;
    };
};


}
}


#endif //KETO_CONSTANTS_HPP
