//
// Created by Brett Chaldecott on 2019/01/22.
//

#ifndef KETO_PROTOCOLCCEPTEDMESSAGEHELPER_HPP
#define KETO_PROTOCOLCCEPTEDMESSAGEHELPER_HPP

#include <vector>
#include <memory>
#include <string>


#include "HandShake.pb.h"

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ProtocolAcceptedMessageHelper;
typedef std::shared_ptr<ProtocolAcceptedMessageHelper> ProtocolAcceptedMessageHelperPtr;


class ProtocolAcceptedMessageHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ProtocolAcceptedMessageHelper();
    ProtocolAcceptedMessageHelper(bool accepted);
    ProtocolAcceptedMessageHelper(const keto::proto::ProtocolAcceptedMessage& msg);
    ProtocolAcceptedMessageHelper(const ProtocolAcceptedMessageHelper& orig) = default;
    virtual ~ProtocolAcceptedMessageHelper();

    ProtocolAcceptedMessageHelper& setAccepted(bool accepted);
    bool getAccepted();

    ProtocolAcceptedMessageHelper& setMsg(const keto::proto::ProtocolAcceptedMessage& msg);
    keto::proto::ProtocolAcceptedMessage getMsg();

    operator keto::proto::ProtocolAcceptedMessage();
    operator std::string();


private:
    keto::proto::ProtocolAcceptedMessage protocolAcceptedMessage;
};


}
}

#endif //KETO_CONSENSUSACCEPTEDMESSAGEHELPER_HPP
