//
// Created by Brett Chaldecott on 2020/07/22.
//

#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/rpc_protocol/NetworkStatusHelper.hpp"
#include "keto/rpc_protocol/Exception.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace rpc_protocol {

NetworkStatusHelper::NetworkStatusHelper() {

}

NetworkStatusHelper::NetworkStatusHelper(const keto::proto::NetworkStatus& networkStatus) : networkStatus(networkStatus) {

}

NetworkStatusHelper::NetworkStatusHelper(const std::string& networkStatus) {
    if (!this->networkStatus.ParseFromString(networkStatus)) {
        BOOST_THROW_EXCEPTION(NetworkStatusDeserializationErrorException());
    }
}

NetworkStatusHelper::~NetworkStatusHelper() {

}

NetworkStatusHelper& NetworkStatusHelper::addAccount(const keto::asn1::HashHelper& account) {
    *this->networkStatus.add_elected_account_hashes() = (std::string)account;
    return *this;
}

std::vector<keto::asn1::HashHelper> NetworkStatusHelper::getAccounts() {
    std::vector<keto::asn1::HashHelper> hashes;
    for (int index = 0; index < this->networkStatus.elected_account_hashes_size(); index++) {
        hashes.push_back(keto::asn1::HashHelper(this->networkStatus.elected_account_hashes(index)));
    }
    return hashes;
}

NetworkStatusHelper& NetworkStatusHelper::setSlot(int slot) {
    this->networkStatus.set_current_slot(slot);
    return *this;
}

int NetworkStatusHelper::getSlot() {
    return this->networkStatus.current_slot();
}

NetworkStatusHelper::operator keto::proto::NetworkStatus() const {
    return this->networkStatus;
}

NetworkStatusHelper::operator std::string() const {
    return this->networkStatus.SerializeAsString();
}

NetworkStatusHelper::operator keto::crypto::SecureVector() const {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            this->networkStatus.SerializeAsString());
}

}
}

