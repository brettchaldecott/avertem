//
// Created by Brett Chaldecott on 2019-09-23.
//

#include <sstream>
#include <ctime>
#include <iomanip>

#include "keto/server_common/DateUtils.hpp"


namespace keto {
namespace server_common {

const char* DateUtils::ISO_8601 = "%Y-%m-%dT%H:%M:%S%Z";

std::string DateUtils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

DateUtils::DateUtils(const std::time_t timestamp) : timestamp(timestamp) {

}

DateUtils::~DateUtils() {

}

std::string DateUtils::formatISO8601() {
    return format(ISO_8601);
}

std::string DateUtils::format(const std::string& format) {
    std::tm tm;
    gmtime_r(&this->timestamp,&tm);
    std::stringstream ss;
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
}

std::time_t DateUtils::getTimestamp() {
    return this->timestamp;
}


}
}