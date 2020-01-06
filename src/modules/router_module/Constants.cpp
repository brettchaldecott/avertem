//
// Created by Brett Chaldecott on 2019-09-20.
//


#include "keto/router/Constants.hpp"


namespace keto {
namespace router {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

// keys for server
const char* Constants::PRIVATE_KEY      = "server-private-key";
const char* Constants::PUBLIC_KEY       = "server-public-key";

const int Constants::DEFAULT_ROUTER_QUEUE_DELAY = 1;

}
}
