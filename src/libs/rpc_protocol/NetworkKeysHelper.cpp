//
// Created by Brett Chaldecott on 2018/12/21.
//

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/rpc_protocol/NetworkKeysHelper.hpp"

#include "keto/rpc_protocol/Exception.hpp"

namespace keto {
namespace rpc_protocol {

NetworkKeysHelper::NetworkKeysHelper() {

}

NetworkKeysHelper::NetworkKeysHelper(const keto::proto::NetworkKeys& networkKeys) : networkKeys(networkKeys) {
}

NetworkKeysHelper::NetworkKeysHelper(const std::string& networkKeys) {
    if (!this->networkKeys.ParseFromString(networkKeys)) {
        BOOST_THROW_EXCEPTION(NetworkKeysDeserializationErrorException());
    }
}

NetworkKeysHelper::~NetworkKeysHelper() {

}

NetworkKeysHelper& NetworkKeysHelper::addNetworkKey(const keto::proto::NetworkKey& networkKey) {
    keto::proto::NetworkKey* newNetworkKey = networkKeys.add_network_keys();
    newNetworkKey->MergeFrom(networkKey);
    return *this;
}

NetworkKeysHelper& NetworkKeysHelper::addNetworkKey(const NetworkKeyHelper& networkKeyHelper) {
    //std::cout << "set the keys" << std::endl;
    keto::proto::NetworkKey* newNetworkKey = networkKeys.add_network_keys();
    //std::cout << "set the network key helper" << std::endl;
    keto::proto::NetworkKey networkKey(networkKeyHelper.getNetworkKey());
    //std::cout << "Copy the network key" << std::endl;
    *newNetworkKey = networkKey;
    //newNetworkKey->MergeFrom((keto::proto::NetworkKey&)networkKeyHelper);
    //std::cout << "add the network key" << std::endl;
    return *this;
}

std::vector<NetworkKeyHelper> NetworkKeysHelper::getNetworkKeys() const {
    std::vector<NetworkKeyHelper> result;
    for (int index = 0; index < networkKeys.network_keys_size(); index++) {
        result.push_back(NetworkKeyHelper(networkKeys.network_keys(index)));
    }
    return result;
}

NetworkKeysHelper::operator keto::proto::NetworkKeys() {
    return this->networkKeys;
}


NetworkKeysHelper::operator std::string() {
    std::string result;
    networkKeys.SerializeToString(&result);
    return result;
}

NetworkKeysHelper::operator keto::crypto::SecureVector() {
    std::string result;
    networkKeys.SerializeToString(&result);
    return keto::crypto::SecureVectorUtils().copyStringToSecure(result);
}

}
}