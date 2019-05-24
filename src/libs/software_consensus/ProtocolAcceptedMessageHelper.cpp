//
// Created by Brett Chaldecott on 2019/01/22.
//

#include "keto/common/MetaInfo.hpp"

#include "keto/software_consensus/ProtocolAcceptedMessageHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ProtocolAcceptedMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ProtocolAcceptedMessageHelper::ProtocolAcceptedMessageHelper() {
    protocolAcceptedMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    protocolAcceptedMessage.set_accepted(true);
}

ProtocolAcceptedMessageHelper::ProtocolAcceptedMessageHelper(bool accepted) {
    protocolAcceptedMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    protocolAcceptedMessage.set_accepted(accepted);
}

ProtocolAcceptedMessageHelper::ProtocolAcceptedMessageHelper(const keto::proto::ProtocolAcceptedMessage &msg) :
        protocolAcceptedMessage(msg) {
}

ProtocolAcceptedMessageHelper::~ProtocolAcceptedMessageHelper() {

}

ProtocolAcceptedMessageHelper& ProtocolAcceptedMessageHelper::setAccepted(bool accepted) {
    this->protocolAcceptedMessage.set_accepted(accepted);
    return *this;
}

bool ProtocolAcceptedMessageHelper::getAccepted() {
    return this->protocolAcceptedMessage.accepted();
}

ProtocolAcceptedMessageHelper& ProtocolAcceptedMessageHelper::setMsg(const keto::proto::ProtocolAcceptedMessage &msg) {
    this->protocolAcceptedMessage = msg;
    return *this;
}

keto::proto::ProtocolAcceptedMessage ProtocolAcceptedMessageHelper::getMsg() {
    return this->protocolAcceptedMessage;
}

ProtocolAcceptedMessageHelper::operator keto::proto::ProtocolAcceptedMessage() {
    return this->protocolAcceptedMessage;
}

ProtocolAcceptedMessageHelper::operator std::string() {
    std::string result;
    this->protocolAcceptedMessage.SerializeToString(&result);
    return result;
}

}
}