//
// Created by Brett Chaldecott on 2019/01/22.
//

#include "keto/common/MetaInfo.hpp"

#include "keto/software_consensus/ProtocolHeartbeatMessageHelper.hpp"

namespace keto {
namespace software_consensus {

std::string ProtocolHeartbeatMessageHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ProtocolHeartbeatMessageHelper::ProtocolHeartbeatMessageHelper() {
    protocolHeartbeatMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    protocolHeartbeatMessage.set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());
}

ProtocolHeartbeatMessageHelper::ProtocolHeartbeatMessageHelper(long timestamp) {
    protocolHeartbeatMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    protocolHeartbeatMessage.set_timestamp(timestamp);
}

ProtocolHeartbeatMessageHelper::ProtocolHeartbeatMessageHelper(const std::chrono::system_clock::time_point& timestamp) {
    protocolHeartbeatMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    protocolHeartbeatMessage.set_timestamp(timestamp.time_since_epoch().count());
}

ProtocolHeartbeatMessageHelper::ProtocolHeartbeatMessageHelper(const keto::proto::ProtocolHeartbeatMessage &msg) :
        protocolHeartbeatMessage(msg) {
}

ProtocolHeartbeatMessageHelper::~ProtocolHeartbeatMessageHelper() {

}

ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setTimestamp(long timestamp) {
    protocolHeartbeatMessage.set_timestamp(timestamp);
    return *this;
}

ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    protocolHeartbeatMessage.set_timestamp(timestamp.time_since_epoch().count());
    return *this;
}

long ProtocolHeartbeatMessageHelper::getTimestamp() {
    return protocolHeartbeatMessage.timestamp();
}

ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setNetworkSlot(int networkSlot) {
    protocolHeartbeatMessage.set_network_slot(networkSlot);
    return *this;
}

int ProtocolHeartbeatMessageHelper::getNetworkSlot() const {
    return protocolHeartbeatMessage.network_slot();
}


ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setElectionSlot(int electionSlot) {
    protocolHeartbeatMessage.set_network_election_slot(electionSlot);
    return *this;
}

int ProtocolHeartbeatMessageHelper::getElectionSlot() const {
    return protocolHeartbeatMessage.network_election_slot();
}

ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setElectionPublishSlot(int electionPublishSlot) {
    protocolHeartbeatMessage.set_network_election_publish_slot(electionPublishSlot);
    return *this;
}

int ProtocolHeartbeatMessageHelper::getElectionPublishSlot() const {
    return protocolHeartbeatMessage.network_election_publish_slot();
}


ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setConfirmationSlot(int confirmationSlot) {
    protocolHeartbeatMessage.set_network_confirmation_slot(confirmationSlot);
    return *this;
}

int ProtocolHeartbeatMessageHelper::getConfirmationSlot() const {
    return protocolHeartbeatMessage.network_confirmation_slot();
}


ProtocolHeartbeatMessageHelper& ProtocolHeartbeatMessageHelper::setMsg(const keto::proto::ProtocolHeartbeatMessage &msg) {
    this->protocolHeartbeatMessage = msg;
    return *this;
}

keto::proto::ProtocolHeartbeatMessage ProtocolHeartbeatMessageHelper::getMsg() {
    return this->protocolHeartbeatMessage;
}

ProtocolHeartbeatMessageHelper::operator keto::proto::ProtocolHeartbeatMessage() {
    return this->protocolHeartbeatMessage;
}

ProtocolHeartbeatMessageHelper::operator std::string() {
    std::string result;
    this->protocolHeartbeatMessage.SerializeToString(&result);
    return result;
}

}
}