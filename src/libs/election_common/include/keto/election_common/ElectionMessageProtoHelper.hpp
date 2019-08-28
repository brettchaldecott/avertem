//
// Created by Brett Chaldecott on 2019-08-20.
//

#ifndef KETO_ELECTIONMESSAGEHELP_HPP
#define KETO_ELECTIONMESSAGEHELP_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.pb.h"

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"

namespace keto {
namespace election_common {

class ElectionMessageProtoHelper;
typedef std::shared_ptr<ElectionMessageProtoHelper> ElectionMessageProtoHelperPtr;

class ElectionMessageProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionMessageProtoHelper();
    ElectionMessageProtoHelper(const keto::proto::ElectionMessage& electionMessage);
    ElectionMessageProtoHelper(const std::string& msg);
    ElectionMessageProtoHelper(const ElectionMessageProtoHelper& electionMessageProtoHelper) = default;
    virtual ~ElectionMessageProtoHelper();


    std::vector<keto::asn1::HashHelper> getAccounts();
    ElectionMessageProtoHelper& addAccount(const keto::asn1::HashHelper& account);

    std::string getSource();
    ElectionMessageProtoHelper& setSource(const std::string& source);

    operator std::string();
    operator keto::proto::ElectionMessage();
    keto::proto::ElectionMessage getElectionMessage();

private:
    keto::proto::ElectionMessage electionMessage;
};


}
}


#endif //KETO_ELECTIONMESSAGEHELP_HPP
