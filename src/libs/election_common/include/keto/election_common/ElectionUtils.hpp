//
// Created by Brett Chaldecott on 2019-08-30.
//

#ifndef KETO_ELECTION_COMMON_ELECTIONUTILS_HPP
#define KETO_ELECTION_COMMON_ELECTIONUTILS_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"

namespace keto {
namespace election_common {

class ElectionUtils;
typedef std::shared_ptr<ElectionUtils> ElectionUtilsPtr;

class ElectionUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionUtils(const std::vector<std::string>& events);
    ElectionUtils(const ElectionUtils& orig) = delete;
    virtual ~ElectionUtils();

    void publish(const ElectionPublishTangleAccountProtoHelperPtr& electionPublishTangleAccountProtoHelperPtr);
    void confirmation();

private:
    std::vector<std::string> events;
};


}
}


#endif //KETO_ELECTIONUTILS_HPP
