//
// Created by Brett Chaldecott on 2019-08-26.
//

#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace election_common {


std::string ElectionPublishTangleAccountProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionPublishTangleAccountProtoHelper::ElectionPublishTangleAccountProtoHelper() {
    this->electionPublishTangleAccount.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ElectionPublishTangleAccountProtoHelper::ElectionPublishTangleAccountProtoHelper(const keto::proto::ElectionPublishTangleAccount& electionPublishTangleAccount) :
    electionPublishTangleAccount(electionPublishTangleAccount) {

}

ElectionPublishTangleAccountProtoHelper::ElectionPublishTangleAccountProtoHelper(const std::string &msg) {
    this->electionPublishTangleAccount.ParseFromString(msg);
}

ElectionPublishTangleAccountProtoHelper::~ElectionPublishTangleAccountProtoHelper() {

}

keto::asn1::HashHelper ElectionPublishTangleAccountProtoHelper::getAccount() {
    return this->electionPublishTangleAccount.account_hash();
}

ElectionPublishTangleAccountProtoHelper& ElectionPublishTangleAccountProtoHelper::setAccount(const keto::asn1::HashHelper &account) {
    this->electionPublishTangleAccount.set_account_hash(account);
    return *this;
}

keto::asn1::HashHelper ElectionPublishTangleAccountProtoHelper::getTangle() {
    return this->electionPublishTangleAccount.tangle_hash();
}

ElectionPublishTangleAccountProtoHelper& ElectionPublishTangleAccountProtoHelper::setTangle(const keto::asn1::HashHelper &tangleHash) {
    this->electionPublishTangleAccount.set_tangle_hash(tangleHash);
    return *this;
}

ElectionPublishTangleAccountProtoHelper::operator keto::proto::ElectionPublishTangleAccount() {
    return this->electionPublishTangleAccount;
}

ElectionPublishTangleAccountProtoHelper::operator std::string() {
    return this->electionPublishTangleAccount.SerializeAsString();
}


}
}