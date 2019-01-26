//
// Created by Brett Chaldecott on 2019/01/25.
//

#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"
#include "keto/rpc_protocol/Exception.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace rpc_protocol {

NetworkKeysWrapperHelper::NetworkKeysWrapperHelper() {

}

NetworkKeysWrapperHelper::NetworkKeysWrapperHelper(const keto::proto::NetworkKeysWrapper& source) :
    networkKeysWrapper(source) {

}

NetworkKeysWrapperHelper::NetworkKeysWrapperHelper(const std::string& source) {
    if (!this->networkKeysWrapper.ParseFromString(source)) {
        BOOST_THROW_EXCEPTION(NetworkKeysWrapperDeserializationErrorException());
    }
}

NetworkKeysWrapperHelper::~NetworkKeysWrapperHelper() {

}


NetworkKeysWrapperHelper& NetworkKeysWrapperHelper::setBytes(const std::vector<uint8_t>& bytes) {
    this->networkKeysWrapper.set_network_keys_bytes(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

std::vector<uint8_t> NetworkKeysWrapperHelper::getBytes() {
    std::vector<uint8_t> result =
            keto::server_common::VectorUtils().copyStringToVector(this->networkKeysWrapper.network_keys_bytes());
    return result;
}

NetworkKeysWrapperHelper::operator std::vector<uint8_t>() {
    std::vector<uint8_t> result =
            keto::server_common::VectorUtils().copyStringToVector(this->networkKeysWrapper.network_keys_bytes());
    return result;
}

NetworkKeysWrapperHelper& NetworkKeysWrapperHelper::operator = (const std::vector<uint8_t>& bytes) {
    this->networkKeysWrapper.set_network_keys_bytes(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

keto::proto::NetworkKeysWrapper NetworkKeysWrapperHelper::getNetworkKeysWrapper() {
    return this->networkKeysWrapper;
}

NetworkKeysWrapperHelper::operator keto::proto::NetworkKeysWrapper() {
    return this->networkKeysWrapper;
}

std::string NetworkKeysWrapperHelper::getNetworkKeysWrapperString() {
    std::string result;
    networkKeysWrapper.SerializeToString(&result);
    return result;
}

}
}
