//
// Created by Brett Chaldecott on 2019/01/22.
//

#include "keto/common/MetaInfo.hpp"

#include "keto/software_consensus/ConsensusAcceptedMessageHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ConsensusAcceptedMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ConsensusAcceptedMessageHelper::ConsensusAcceptedMessageHelper() {
    consensusAcceptedMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    consensusAcceptedMessage.set_accepted(true);
}

ConsensusAcceptedMessageHelper::ConsensusAcceptedMessageHelper(bool accepted) {
    consensusAcceptedMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    consensusAcceptedMessage.set_accepted(accepted);
}

ConsensusAcceptedMessageHelper::ConsensusAcceptedMessageHelper(const keto::proto::ConsensusAcceptedMessage &msg) :
        consensusAcceptedMessage(msg) {
}

ConsensusAcceptedMessageHelper::~ConsensusAcceptedMessageHelper() {

}

ConsensusAcceptedMessageHelper& ConsensusAcceptedMessageHelper::setAccepted(bool accepted) {
    this->consensusAcceptedMessage.set_accepted(accepted);
    return *this;
}

bool ConsensusAcceptedMessageHelper::getAccepted() {
    return this->consensusAcceptedMessage.accepted();
}

ConsensusAcceptedMessageHelper& ConsensusAcceptedMessageHelper::setMsg(const keto::proto::ConsensusAcceptedMessage &msg) {
    this->consensusAcceptedMessage = msg;
    return *this;
}

keto::proto::ConsensusAcceptedMessage ConsensusAcceptedMessageHelper::getMsg() {
    return this->consensusAcceptedMessage;
}

ConsensusAcceptedMessageHelper::operator keto::proto::ConsensusAcceptedMessage() {
    return this->consensusAcceptedMessage;
}

ConsensusAcceptedMessageHelper::operator std::string() {
    std::string result;
    this->consensusAcceptedMessage.SerializeToString(&result);
    return result;
}

}
}