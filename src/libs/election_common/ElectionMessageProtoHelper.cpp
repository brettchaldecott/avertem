//
// Created by Brett Chaldecott on 2019-08-20.
//

#include "keto/election_common/ElectionMessageProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace election_common {

std::string ElectionMessageProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


ElectionMessageProtoHelper::ElectionMessageProtoHelper() {
    this->electionMessage.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ElectionMessageProtoHelper::ElectionMessageProtoHelper(const keto::proto::ElectionMessage& electionMessage) :
    electionMessage(electionMessage) {

}

ElectionMessageProtoHelper::ElectionMessageProtoHelper(const std::string& msg) {
    this->electionMessage.ParseFromString(msg);
}

ElectionMessageProtoHelper::~ElectionMessageProtoHelper() {
}


std::vector<keto::asn1::HashHelper> ElectionMessageProtoHelper::getAccounts() {
    std::vector<keto::asn1::HashHelper> hashes;
    for (int index = 0; index < this->electionMessage.account_hashes_size(); index++) {
        hashes.push_back(keto::asn1::HashHelper(this->electionMessage.account_hashes(index)));
    }
    return hashes;
}

ElectionMessageProtoHelper& ElectionMessageProtoHelper::addAccount(const keto::asn1::HashHelper& account) {
    *this->electionMessage.add_account_hashes() = (std::string&)account;
    return *this;
}

std::string ElectionMessageProtoHelper::getSource() {
    return this->electionMessage.source();
}

ElectionMessageProtoHelper& ElectionMessageProtoHelper::setSource(const std::string& source) {
    this->electionMessage.set_source(source);
    return *this;
}

ElectionMessageProtoHelper::operator std::string() {
    return this->electionMessage.SerializeAsString();
}

ElectionMessageProtoHelper::operator keto::proto::ElectionMessage() {
    return this->electionMessage;
}

keto::proto::ElectionMessage ElectionMessageProtoHelper::getElectionMessage() {
    return this->electionMessage;
}


}
}
