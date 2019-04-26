//
// Created by Brett Chaldecott on 2019/03/05.
//

#include "keto/key_store_utils/EncryptionNetworkRequestProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {


std::string EncryptionNetworkRequestProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EncryptionNetworkRequestProtoHelper::EncryptionNetworkRequestProtoHelper() {
    this->encryptNetworkRequest.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

EncryptionNetworkRequestProtoHelper::EncryptionNetworkRequestProtoHelper(const std::string& value) {
    this->encryptNetworkRequest.ParseFromString(value);
}

EncryptionNetworkRequestProtoHelper::EncryptionNetworkRequestProtoHelper(const std::vector<uint8_t>& bytes) {
    this->encryptNetworkRequest.set_bytes_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
}

EncryptionNetworkRequestProtoHelper::EncryptionNetworkRequestProtoHelper(const keto::proto::EncryptNetworkRequest& encryptNetworkRequest) : encryptNetworkRequest(encryptNetworkRequest){

}

EncryptionNetworkRequestProtoHelper::~EncryptionNetworkRequestProtoHelper() {

}

EncryptionNetworkRequestProtoHelper& EncryptionNetworkRequestProtoHelper::setByteMessage(const std::vector<uint8_t>& bytes) {
    this->encryptNetworkRequest.set_bytes_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionNetworkRequestProtoHelper& EncryptionNetworkRequestProtoHelper::operator = (const std::vector<uint8_t>& bytes) {
    this->encryptNetworkRequest.set_bytes_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionNetworkRequestProtoHelper& EncryptionNetworkRequestProtoHelper::setByteMessage(const keto::crypto::SecureVector& bytes) {
    this->encryptNetworkRequest.set_bytes_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionNetworkRequestProtoHelper& EncryptionNetworkRequestProtoHelper::operator = (const keto::crypto::SecureVector& bytes) {
    this->encryptNetworkRequest.set_bytes_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionNetworkRequestProtoHelper::operator std::vector<uint8_t>() const {
    return keto::server_common::VectorUtils().copyStringToVector(this->encryptNetworkRequest.bytes_message());
}

EncryptionNetworkRequestProtoHelper::operator keto::crypto::SecureVector() const {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(this->encryptNetworkRequest.bytes_message());
}

EncryptionNetworkRequestProtoHelper::operator keto::proto::EncryptNetworkRequest() const {
    return this->encryptNetworkRequest;
}

}
}