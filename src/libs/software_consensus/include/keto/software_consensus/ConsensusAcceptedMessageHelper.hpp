//
// Created by Brett Chaldecott on 2019/01/22.
//

#ifndef KETO_CONSENSUSACCEPTEDMESSAGEHELPER_HPP
#define KETO_CONSENSUSACCEPTEDMESSAGEHELPER_HPP

#include <vector>
#include <memory>
#include <string>


#include "HandShake.pb.h"

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ConsensusAcceptedMessageHelper;
typedef std::shared_ptr<ConsensusAcceptedMessageHelper> ConsensusAcceptedMessageHelperPtr;


class ConsensusAcceptedMessageHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ConsensusAcceptedMessageHelper();
    ConsensusAcceptedMessageHelper(bool accepted);
    ConsensusAcceptedMessageHelper(const keto::proto::ConsensusAcceptedMessage& msg);
    ConsensusAcceptedMessageHelper(const ConsensusAcceptedMessageHelper& orig) = default;
    virtual ~ConsensusAcceptedMessageHelper();

    ConsensusAcceptedMessageHelper& setAccepted(bool accepted);
    bool getAccepted();

    ConsensusAcceptedMessageHelper& setMsg(const keto::proto::ConsensusAcceptedMessage& msg);
    keto::proto::ConsensusAcceptedMessage getMsg();

    operator keto::proto::ConsensusAcceptedMessage();
    operator std::string();


private:
    keto::proto::ConsensusAcceptedMessage consensusAcceptedMessage;
};


}
}

#endif //KETO_CONSENSUSACCEPTEDMESSAGEHELPER_HPP
