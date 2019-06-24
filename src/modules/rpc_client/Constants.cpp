//
// Created by Brett Chaldecott on 2019-05-05.
//


#include "keto/rpc_client/Constants.hpp"

namespace keto {
namespace rpc_client {

const int Constants::SESSION::MAX_RETRY_COUNT = 120;
const long Constants::SESSION::RETRY_COUNT_DELAY = 10000;


const char* Constants::PEER_INDEX = "peers";


const std::vector<std::string> Constants::DB_LIST =
        {Constants::PEER_INDEX};
}
}

