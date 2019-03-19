//
// Created by Brett Chaldecott on 2019/03/19.
//

#ifndef KETO_RDFUTILS_HPP
#define KETO_RDFUTILS_HPP

#include <string>

namespace keto {
namespace server_common {

class RDFUtils {
public:
    static std::string convertTimeToRDFDateTime(const time_t value);
    static time_t convertRDFDateTimeToTime(const std::string& value);
};


}
}



#endif //KETO_RDFUTILS_HPP
