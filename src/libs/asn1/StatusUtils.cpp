//
// Created by Brett Chaldecott on 2019/02/14.
//

#include "keto/asn1/StatusUtils.hpp"


namespace keto {
namespace asn1 {

std::string StatusUtils::statusToString(Status_t status) {
    if (status == Status_init) {
        return "INIT";
    } else if (status == Status_debit) {
        return "DEBIT";
    } else if (status == Status_credit) {
        return "CREDIT";
    } else if (status == Status_fee) {
        return "FEE";
    } else if (status == Status_processing) {
        return "PROCESSING";
    } else {
        return "UNKNOWN";
    }
}

}
}