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
    networkKey.set_key_hash(hash.data(),hash.size());
    return *this;
}

keto::crypto::SecureVector NetworkKeyHelper::getHash_locked() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            networkKey.key_hash());
}

NetworkKeyHelper& NetworkKeyHelper::setHash(const keto::crypto::SecureVector& hash) {
    networkKey.set_key_hash(hash.data(),hash.size());
    return *this;
}


keto::crypto::SecureVector NetworkKeyHelper::getKeyBytes() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            networkKey.network_key());
}


NetworkKeyHelper& NetworkKeyHelper::setKeyBytes(const keto::crypto::SecureVector& bytes) {
    networkKey.set_network_key(bytes.data(),bytes.size());
    return *this;
}


NetworkKeyHelper::operator keto::proto::NetworkKey() {
    return networkKey;
}

}
};
