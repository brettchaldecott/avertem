//
// Created by Brett Chaldecott on 2019/03/22.
//

#include "keto/cli/WallyScope.hpp"
#include "wally.hpp"

namespace keto {
namespace cli {

WallyScope::WallyScope() {
    wally_init(0);
}

WallyScope::~WallyScope() {
    wally_cleanup(0);
}


}
}