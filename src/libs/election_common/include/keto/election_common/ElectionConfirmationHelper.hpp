//
// Created by Brett Chaldecott on 2019-09-01.
//

#ifndef KETO_ELECTIONCONFIRMATIONHELPER_HPP
#define KETO_ELECTIONCONFIRMATIONHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.pb.h"

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"

namespace keto {
namespace election_common {

class ElectionConfirmationHelper;
typedef std::shared_ptr<ElectionConfirmationHelper> ElectionConfirmationHelperPtr;

class ElectionConfirmationHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionConfirmationHelper();
    ElectionConfirmationHelper(const keto::proto::ElectionConfirmation& electionConfirmation);
    ElectionConfirmationHelper(const std::string& msg);
    ElectionConfirmationHelper(const ElectionConfirmationHelper& orig) = default;
    virtual ~ElectionConfirmationHelper();

    operator keto::proto::ElectionConfirmation() const;
    operator std::string() const;

private:
    keto::proto::ElectionConfirmation electionConfirmation;

};

}
}

#endif //KETO_ELECTIONCONFIRMATIONHELPER_HPP
