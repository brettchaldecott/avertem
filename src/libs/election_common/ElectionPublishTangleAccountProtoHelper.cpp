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
    setGrowing(true);
}

ElectionPublishTangleAccountProtoHelper::ElectionPublishTangleAccountProtoHelper(const keto::proto::ElectionPublishTangleAccount& electionPublishTangleAccount) :
    electionPublishTangleAccount(electionPublishTangleAccount) {

}

ElectionPublishTangleAccountProtoHelper::ElectionPublishTangleAccountProtoHelper(const std::string &msg) {
    this->electionPublishTangleAccount.ParseFromString(msg);
}

ElectionPublishTangleAccountProtoHelper::~ElectionPublishTangleAccountProtoHelper() {

}

keto::asn1::HashHelper ElectionPublishTangleAccountProtoHelper::getAccount() const {
    return this->electionPublishTangleAccount.account_hash();
}

ElectionPublishTangleAccountProtoHelper& ElectionPublishTangleAccountProtoHelper::setAccount(const keto::asn1::HashHelper &account) {
    this->electionPublishTangleAccount.set_account_hash(account);
    return *this;
}

std::vector<keto::asn1::HashHelper> ElectionPublishTangleAccountProtoHelper::getTangles() const {
    std::vector<keto::asn1::HashHelper> hashes;
    for (int index = 0; index < this->electionPublishTangleAccount.tangle_hashes_size(); index++) {
        hashes.push_back(keto::asn1::HashHelper(this->electionPublishTangleAccount.tangle_hashes(index)));
    }
    return hashes;
}

ElectionPublishTangleAccountProtoHelper& ElectionPublishTangleAccountProtoHelper::addTangle(const keto::asn1::HashHelper& tangleHash) {
    *this->electionPublishTangleAccount.add_tangle_hashes() = (std::string&)tangleHash;
    return *this;
}

int ElectionPublishTangleAccountProtoHelper::size() {
    return this->electionPublishTangleAccount.tangle_hashes_size();
}

bool ElectionPublishTangleAccountProtoHelper::isGrowing() const {
    return this->electionPublishTangleAccount.growing();
}

ElectionPublishTangleAccountProtoHelper& ElectionPublishTangleAccountProtoHelper::setGrowing(bool growing) {
    this->electionPublishTangleAccount.set_growing(growing);
    return *this;
}


ElectionPublishTangleAccountProtoHelper::operator keto::proto::ElectionPublishTangleAccount() const {
    return this->electionPublishTangleAccount;
}

ElectionPublishTangleAccountProtoHelper::operator std::string() const {
    return this->electionPublishTangleAccount.SerializeAsString();
}


}
}