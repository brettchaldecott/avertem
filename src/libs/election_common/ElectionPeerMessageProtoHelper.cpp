//
// Created by Brett Chaldecott on 2019-08-23.
//

#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace election_common {

std::string ElectionPeerMessageProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionPeerMessageProtoHelper::ElectionPeerMessageProtoHelper() {
    this->electionPeerMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ElectionPeerMessageProtoHelper::ElectionPeerMessageProtoHelper(const keto::proto::ElectionPeerMessage& electionPeerMessage) :
        electionPeerMessage(electionPeerMessage) {
}

ElectionPeerMessageProtoHelper::ElectionPeerMessageProtoHelper(const std::string &msg) {
    this->electionPeerMessage.ParseFromString(msg);
}

ElectionPeerMessageProtoHelper::~ElectionPeerMessageProtoHelper() {

}

keto::asn1::HashHelper ElectionPeerMessageProtoHelper::getAccount() {
    return this->electionPeerMessage.account_hash();
}

ElectionPeerMessageProtoHelper& ElectionPeerMessageProtoHelper::setAccount(const keto::asn1::HashHelper &account) {
    this->electionPeerMessage.set_account_hash(account);
    return *this;
}

keto::asn1::HashHelper ElectionPeerMessageProtoHelper::getPeer() {
    return this->electionPeerMessage.peer_hash();
}

ElectionPeerMessageProtoHelper& ElectionPeerMessageProtoHelper::setPeer(const keto::asn1::HashHelper& peer) {
    this->electionPeerMessage.set_peer_hash(peer);
    return *this;
}

ElectionPeerMessageProtoHelper::operator keto::proto::ElectionPeerMessage() {
    return this->electionPeerMessage;
}

ElectionPeerMessageProtoHelper::operator std::string() {
    return this->electionPeerMessage.SerializeAsString();
}

}
}