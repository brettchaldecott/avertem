    //
// Created by Brett Chaldecott on 2020/08/04.
//

#ifndef KETO_PUBLISHEDELECTIONINFORMATIONHELPER_HPP
#define KETO_PUBLISHEDELECTIONINFORMATIONHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.pb.h"

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"

#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"

namespace keto {
namespace election_common {

class PublishedElectionInformationHelper;
typedef std::shared_ptr <PublishedElectionInformationHelper> PublishedElectionInformationHelperPtr;

class PublishedElectionInformationHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    PublishedElectionInformationHelper();
    PublishedElectionInformationHelper(const keto::proto::PublishedElectionInformation& publishedElectionInformation);
    PublishedElectionInformationHelper(const std::string& msg);
    PublishedElectionInformationHelper(const PublishedElectionInformationHelper& orig) = default;
    virtual ~PublishedElectionInformationHelper();

    std::vector<ElectionPublishTangleAccountProtoHelperPtr> getElectionPublishTangleAccounts() const;
    PublishedElectionInformationHelper& addElectionPublishTangleAccount(const ElectionPublishTangleAccountProtoHelperPtr& electionPublishTangleAccountProtoHelperPtr);
    int size();

    operator keto::proto::PublishedElectionInformation() const;
    operator std::string() const;

private:
    keto::proto::PublishedElectionInformation publishedElectionInformation;
};

}
}


#endif //KETO_PUBLISHEDELECTIONINFORMATIONHELPER_HPP
