//
// Created by Brett Chaldecott on 2019-09-01.
//

#include "keto/election_common/ElectionConfirmationHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace election_common {

std::string ElectionConfirmationHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionConfirmationHelper::ElectionConfirmationHelper() {
    this->electionConfirmation.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ElectionConfirmationHelper::ElectionConfirmationHelper(const keto::proto::ElectionConfirmation& electionConfirmation) :
    electionConfirmation(electionConfirmation) {

}

ElectionConfirmationHelper::ElectionConfirmationHelper(const std::string& msg) {
    this->electionConfirmation.ParseFromString(msg);
}

ElectionConfirmationHelper::~ElectionConfirmationHelper() {

}

ElectionConfirmationHelper::operator keto::proto::ElectionConfirmation() const {
    return this->electionConfirmation;
}

ElectionConfirmationHelper::operator std::string() const {
    return this->electionConfirmation.SerializeAsString();
}


}
}
