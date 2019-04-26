//
// Created by Brett Chaldecott on 2019/03/05.
//

#include "keto/key_store_utils/EncryptionNetworkResponseProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {


std::string EncryptionNetworkResponseProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EncryptionNetworkResponseProtoHelper::EncryptionNetworkResponseProtoHelper() {
    this->encryptNetworkResponse.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

EncryptionNetworkResponseProtoHelper::EncryptionNetworkResponseProtoHelper(const std::string& value) {
    this->encryptNetworkResponse.ParseFromString(value);
}

EncryptionNetworkResponseProtoHelper::EncryptionNetworkResponseProtoHelper(const keto::proto::EncryptNetworkResponse& encryptNetworkResponse) : encryptNetworkResponse(encryptNetworkResponse){

}

EncryptionNetworkResponseProtoHelper::~EncryptionNetworkResponseProtoHelper() {

}

EncryptionNetworkResponseProtoHelper& EncryptionNetworkResponseProtoHelper::setBytesMessage(const std::vector<uint8_t>& bytes) {
    this->encryptNetworkResponse.set_bytes_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionNetworkResponseProtoHelper& EncryptionNetworkResponseProtoHelper::operator = (const std::vector<uint8_t>& bytes) {
    this->encryptNetworkResponse.set_bytes_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionNetworkResponseProtoHelper& EncryptionNetworkResponseProtoHelper::setBytesMessage(const keto::crypto::SecureVector& bytes) {
    this->encryptNetworkResponse.set_bytes_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionNetworkResponseProtoHelper& EncryptionNetworkResponseProtoHelper::operator = (const keto::crypto::SecureVector& bytes) {
    this->encryptNetworkResponse.set_bytes_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionNetworkResponseProtoHelper::operator std::string() const {
    return this->encryptNetworkResponse.bytes_message();
}

EncryptionNetworkResponseProtoHelper::operator std::vector<uint8_t>() const {
    return keto::server_common::VectorUtils().copyStringToVector(this->encryptNetworkResponse.bytes_message());
}

EncryptionNetworkResponseProtoHelper::operator keto::crypto::SecureVector() const {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(this->encryptNetworkResponse.bytes_message());
}

EncryptionNetworkResponseProtoHelper::operator keto::proto::EncryptNetworkResponse() const {
    return this->encryptNetworkResponse;
}

}
}