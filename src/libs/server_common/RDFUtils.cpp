//
// Created by Brett Chaldecott on 2019/03/19.
//

#include "keto/server_common/RDFUtils.hpp"

#include <time.h>

namespace keto {
namespace server_common {

std::string RDFUtils::convertTimeToRDFDateTime(const time_t value) {
    struct tm  tstruct;
    char       buf[80];
    struct tm* result;
    result = localtime_r(&value,&tstruct);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%T", result);
    return buf;
}

time_t RDFUtils::convertRDFDateTimeToTime(const std::string& value) {
    struct tm tm;
    strptime(value.c_str(), "%Y-%m-%dT%X", &tm);
    return mktime(&tm);
}

}
}