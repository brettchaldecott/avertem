//
// Created by Brett Chaldecott on 2019-08-21.
//

#ifndef KETO_ELECTIONRESULTPROTOHELPER_HPP
#define KETO_ELECTIONRESULTPROTOHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.pb.h"


#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "SignedElectionHelper.hpp"


namespace keto {
namespace election_common {

class ElectionResultMessageProtoHelper;
typedef std::shared_ptr<ElectionResultMessageProtoHelper> ElectionResultMessageProtoHelperPtr;

class ElectionResultMessageProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionResultMessageProtoHelper();
    ElectionResultMessageProtoHelper(const keto::proto::ElectionResultMessage& electionResultMessage);
    ElectionResultMessageProtoHelper(const std::string& msg);
    ElectionResultMessageProtoHelper(const ElectionResultMessageProtoHelper& electionResultMessageProtoHelper) = default;
    virtual ~ElectionResultMessageProtoHelper();


    keto::asn1::HashHelper getSourceAccountHash();
    ElectionResultMessageProtoHelper& setSourceAccountHash(const keto::asn1::HashHelper& sourceAccountHash);

    ElectionResultMessageProtoHelper& setElectionMsg(const SignedElectionHelper& signedElectionHelper);
    SignedElectionHelper getElectionMsg();

    operator keto::proto::ElectionResultMessage();
    operator std::string();
    keto::proto::ElectionResultMessage getElectionResultMessage();

private:
    keto::proto::ElectionResultMessage electionResultMessage;

};

}
}


#endif //KETO_ELECTIONRESULTPROTOHELPER_HPP
