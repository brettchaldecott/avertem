//
// Created by Brett Chaldecott on 2019-09-20.
//

#ifndef KETO_ROUTERMODULE_CONSTANTS_HPP
#define KETO_ROUTERMODULE_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace router {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    // keys for server
    static const char* PRIVATE_KEY;
    static const char* PUBLIC_KEY;

};

}
}

#endif //KETO_CONSTANTS_HPP
