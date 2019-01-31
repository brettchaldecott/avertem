//
// Created by Brett Chaldecott on 2018/12/21.
//

#include "keto/rpc_protocol/NetworkKeyHelper.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace rpc_protocol {

NetworkKeyHelper::NetworkKeyHelper() {
}

NetworkKeyHelper::NetworkKeyHelper(const keto::proto::NetworkKey& networkKey) : networkKey(networkKey) {
}

NetworkKeyHelper::~NetworkKeyHelper() {

}


std::vector<uint8_t> NetworkKeyHelper::getHash() {
    return keto::server_common::VectorUtils().copyStringToVector(
            networkKey.key_hash());
}

NetworkKeyHelper& NetworkKeyHelper::setHash(const std::vector<uint8_t>& hash) {
    networkKey.set_key_hash(keto::server_common::VectorUtils().copyVectorToString(hash));
    return *this;
}

keto::crypto::SecureVector NetworkKeyHelper::getHash_locked() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            networkKey.key_hash());
}

NetworkKeyHelper& NetworkKeyHelper::setHash(const keto::crypto::SecureVector& hash) {
    networkKey.set_key_hash(keto::crypto::SecureVectorUtils().copySecureToString(hash));
    return *this;
}

std::vector<uint8_t> NetworkKeyHelper::getKeyBytes() {
    return keto::server_common::VectorUtils().copyStringToVector(
            networkKey.network_key());
}

NetworkKeyHelper& NetworkKeyHelper::setKeyBytes(const std::vector<uint8_t>& bytes) {
    networkKey.set_network_key(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

keto::crypto::SecureVector NetworkKeyHelper::getKeyBytes_locked() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            networkKey.network_key());
}


NetworkKeyHelper& NetworkKeyHelper::setKeyBytes(const keto::crypto::SecureVector& bytes) {
    networkKey.set_network_key(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}


bool NetworkKeyHelper::getActive() {
    return networkKey.active();
}

NetworkKeyHelper& NetworkKeyHelper::setActive(bool active) {
    networkKey.set_active(active);
    return *this;
}


keto::proto::NetworkKey NetworkKeyHelper::getNetworkKey() const {
    //std::cout << "return the network key" << std::endl;
    return networkKey;
}

NetworkKeyHelper::operator keto::proto::NetworkKey() const {
    return networkKey;
}

}
};
