//
// Created by Brett Chaldecott on 2018/12/21.
//

#include "keto/rpc_protocol/NetworkKeyHelper.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace rpc_protocol {

NetworkKeyHelper::NetworkKeyHelper() {
}

NetworkKeyHelper::NetworkKeyHelper(const keto::proto::NetworkKey& networkKey) : networkKey(networkKey) {
}

NetworkKeyHelper::~NetworkKeyHelper() {

}


keto::crypto::SecureVector NetworkKeyHelper::getAccountHash() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            networkKey.master_node_account_hash());
}


NetworkKeyHelper& NetworkKeyHelper::setAccountHash(const keto::crypto::SecureVector& accountHash) {
    networkKey.set_master_node_account_hash(accountHash.data(),accountHash.size());
    return *this;
}


keto::crypto::SecureVector NetworkKeyHelper::getKeyBytes() {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(
            networkKey.master_node_account_key());
}


NetworkKeyHelper& NetworkKeyHelper::setKeyBytes(const keto::crypto::SecureVector& bytes) {
    networkKey.set_master_node_account_hash(bytes.data(),bytes.size());
    return *this;
}


NetworkKeyHelper::operator keto::proto::NetworkKey() {
    return networkKey;
}

}
};
