//
// Created by Brett Chaldecott on 2019-09-23.
//

#ifndef KETO_DATEUTILS_HPP
#define KETO_DATEUTILS_HPP

#include <string>

#include "keto/obfuscate/MetaString.hpp"



namespace keto {
namespace server_common {


class DateUtils {
public:
    static const char* ISO_8601;

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    DateUtils(const std::time_t timestamp);
    DateUtils(const DateUtils& orig) = default;
    virtual ~DateUtils();

    std::string formatISO8601();
    std::string format(const std::string& format);

    std::time_t getTimestamp();

private:
    std::time_t timestamp;
};


}
}


#endif //KETO_DATEUTILS_HPP
