//
// Created by Brett Chaldecott on 2019/02/14.
//

#ifndef KETO_STATUSUTILS_HPP
#define KETO_STATUSUTILS_HPP

#include <string>
#include <memory>

#include "Status.h"


namespace keto {
namespace asn1 {


class StatusUtils {
public:
    static std::string statusToString(Status_t status);
};


}
}


#endif //KETO_STATUSUTILS_HPP
