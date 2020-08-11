//
// Created by Brett Chaldecott on 2020/08/04.
//

#include "keto/election_common/PublishedElectionInformationHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace election_common {

std::string PublishedElectionInformationHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

PublishedElectionInformationHelper::PublishedElectionInformationHelper() {
    this->publishedElectionInformation.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

PublishedElectionInformationHelper::PublishedElectionInformationHelper(const keto::proto::PublishedElectionInformation& publishedElectionInformation) :
    publishedElectionInformation(publishedElectionInformation) {

}

PublishedElectionInformationHelper::PublishedElectionInformationHelper(const std::string& msg) {
    this->publishedElectionInformation.ParseFromString(msg);
}

PublishedElectionInformationHelper::~PublishedElectionInformationHelper() {

}

std::vector<ElectionPublishTangleAccountProtoHelperPtr> PublishedElectionInformationHelper::getElectionPublishTangleAccounts() const {
    std::vector<ElectionPublishTangleAccountProtoHelperPtr> electionPublishTangleAccounts;
    for (int index = 0; index < this->publishedElectionInformation.election_publish_tangle_accounts_size(); index++) {
        electionPublishTangleAccounts.push_back(ElectionPublishTangleAccountProtoHelperPtr(
                new ElectionPublishTangleAccountProtoHelper(
                        this->publishedElectionInformation.election_publish_tangle_accounts(index))));
    }
    return electionPublishTangleAccounts;
}

PublishedElectionInformationHelper& PublishedElectionInformationHelper::addElectionPublishTangleAccount(const ElectionPublishTangleAccountProtoHelperPtr& electionPublishTangleAccountProtoHelperPtr) {
    *this->publishedElectionInformation.add_election_publish_tangle_accounts() = (keto::proto::ElectionPublishTangleAccount)(*electionPublishTangleAccountProtoHelperPtr);
    return *this;
}

int PublishedElectionInformationHelper::size() {
    return this->publishedElectionInformation.election_publish_tangle_accounts_size();
}

PublishedElectionInformationHelper::operator keto::proto::PublishedElectionInformation() const {
    return this->publishedElectionInformation;
}

PublishedElectionInformationHelper::operator std::string() const {
    return this->publishedElectionInformation.SerializeAsString();
}

}
}