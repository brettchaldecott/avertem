//
// Created by Brett Chaldecott on 2019-09-19.
//

#ifndef KETO_CHAINQUERYCOMMON_CONSTANTS_HPP
#define KETO_CHAINQUERYCOMMON_CONSTANTS_HPP

#include <string>
#include <vector>
#include <memory>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace chain_query_common {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static const long DEFAULT_NUMBER_OF_BLOCKS;
    static const long MAX_NUMBER_OF_BLOCKS;
};

}
}

#endif //KETO_CONSTANTS_HPP
