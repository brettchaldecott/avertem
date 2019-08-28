//
// Created by Brett Chaldecott on 2019-08-21.
//

#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace election_common {

std::string ElectionResultMessageProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionResultMessageProtoHelper::ElectionResultMessageProtoHelper() {
    this->electionResultMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ElectionResultMessageProtoHelper::ElectionResultMessageProtoHelper(const keto::proto::ElectionResultMessage& electionResultMessage) :
        electionResultMessage(electionResultMessage) {

}

ElectionResultMessageProtoHelper::ElectionResultMessageProtoHelper(const std::string& msg){
    this->electionResultMessage.ParseFromString(msg);
}

ElectionResultMessageProtoHelper::~ElectionResultMessageProtoHelper() {

}

ElectionResultMessageProtoHelper& ElectionResultMessageProtoHelper::setElectionMsg(const SignedElectionHelper& signedElectionHelper) {
    this->electionResultMessage.set_elecion_msg(signedElectionHelper);
    return *this;
}

SignedElectionHelper ElectionResultMessageProtoHelper::getElectionMsg() {
    return SignedElectionHelper(this->electionResultMessage.elecion_msg());
}

keto::asn1::HashHelper ElectionResultMessageProtoHelper::getSourceAccountHash() {
    return this->electionResultMessage.source_account_hash();
}

ElectionResultMessageProtoHelper& ElectionResultMessageProtoHelper::setSourceAccountHash(const keto::asn1::HashHelper& sourceAccountHash) {
    this->electionResultMessage.set_source_account_hash(sourceAccountHash);
    return *this;
}

ElectionResultMessageProtoHelper::operator keto::proto::ElectionResultMessage(){
    return this->electionResultMessage;
}

ElectionResultMessageProtoHelper::operator std::string() {
    return this->electionResultMessage.SerializeAsString();
}

keto::proto::ElectionResultMessage ElectionResultMessageProtoHelper::getElectionResultMessage() {
    return this->electionResultMessage;
}


}
}

