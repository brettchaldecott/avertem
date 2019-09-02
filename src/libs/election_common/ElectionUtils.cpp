//
// Created by Brett Chaldecott on 2019-08-30.
//

#include "keto/election_common/ElectionUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/election_common/ElectionConfirmationHelper.hpp"

namespace keto {
namespace election_common {

std::string ElectionUtils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionUtils::ElectionUtils(const std::vector<std::string> &events) {

}


ElectionUtils::~ElectionUtils() {

}

void ElectionUtils::publish(const ElectionPublishTangleAccountProtoHelperPtr &electionPublishTangleAccountProtoHelperPtr) {
    for (std::string event : events) {
        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::ElectionPublishTangleAccount>(
                        event,*electionPublishTangleAccountProtoHelperPtr));
    }
}


void ElectionUtils::confirmation() {
    for (std::string event : events) {
        keto::server_common::triggerEvent(
                keto::server_common::toEvent<keto::proto::ElectionConfirmation>(
                        event,ElectionConfirmationHelper()));
    }
}

}
}