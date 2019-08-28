//
// Created by Brett Chaldecott on 2019-08-23.
//

#ifndef KETO_ELECTIONPEERMESSAGEPROTOHELPER_HPP
#define KETO_ELECTIONPEERMESSAGEPROTOHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.pb.h"

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"


namespace keto {
namespace election_common {

class ElectionPeerMessageProtoHelper;
typedef std::shared_ptr <ElectionPeerMessageProtoHelper> ElectionPeerMessageProtoHelperPtr;


class ElectionPeerMessageProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionPeerMessageProtoHelper();
    ElectionPeerMessageProtoHelper(const keto::proto::ElectionPeerMessage& electionPeerMessage);
    ElectionPeerMessageProtoHelper(const std::string& msg);
    ElectionPeerMessageProtoHelper(const ElectionPeerMessageProtoHelper& orig) = default;
    virtual ~ElectionPeerMessageProtoHelper();

    keto::asn1::HashHelper getAccount();
    ElectionPeerMessageProtoHelper& setAccount(const keto::asn1::HashHelper& account);

    keto::asn1::HashHelper getPeer();
    ElectionPeerMessageProtoHelper& setPeer(const keto::asn1::HashHelper& peer);

    operator keto::proto::ElectionPeerMessage();
    operator std::string();

private:
    keto::proto::ElectionPeerMessage electionPeerMessage;

};

}
}


#endif //KETO_ELECTIONPEERMESSAGEPROTOHELPER_HPP
