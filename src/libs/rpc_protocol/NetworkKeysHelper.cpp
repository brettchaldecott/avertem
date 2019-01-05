//
// Created by Brett Chaldecott on 2018/12/21.
//

#include "keto/rpc_protocol/NetworkKeysHelper.hpp"

namespace keto {
namespace rpc_protocol {

NetworkKeysHelper::NetworkKeysHelper() {

}

NetworkKeysHelper::NetworkKeysHelper(const keto::proto::NetworkKeys& networkKeys) : networkKeys(networkKeys) {
}

NetworkKeysHelper::~NetworkKeysHelper() {

}

NetworkKeysHelper& NetworkKeysHelper::addNetworkKey(const keto::proto::NetworkKey& networkKey) {
    keto::proto::NetworkKey* newNetworkKey = networkKeys.add_network_keys();
    newNetworkKey->CopyFrom(networkKey);
    return *this;
}

NetworkKeysHelper& NetworkKeysHelper::addNetworkKey(const NetworkKeyHelper& networkKeyHelper) {
    keto::proto::NetworkKey* newNetworkKey = networkKeys.add_network_keys();
    newNetworkKey->CopyFrom((keto::proto::NetworkKey&)networkKeyHelper);
    return *this;
}

std::vector<NetworkKeyHelper> NetworkKeysHelper::getNetworkKeys() {
    std::vector<NetworkKeyHelper> result;
    for (int index = 0; index < networkKeys.network_keys_size(); index++) {
        result.push_back(NetworkKeyHelper(networkKeys.network_keys(index)));
    }
    return result;
}

NetworkKeysHelper::operator keto::proto::NetworkKeys() {
    return this->networkKeys;
}



}
}