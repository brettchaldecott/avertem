//
// Created by Brett Chaldecott on 2019/03/05.
//

#include "keto/key_store_utils/EncryptionRequestProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {


std::string EncryptionRequestProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EncryptionRequestProtoHelper::EncryptionRequestProtoHelper() {
    this->encryptRequest.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

EncryptionRequestProtoHelper::EncryptionRequestProtoHelper(const std::string& value) {
    this->encryptRequest.ParseFromString(value);
}

EncryptionRequestProtoHelper::EncryptionRequestProtoHelper(const keto::proto::EncryptRequest encryptRequest) : encryptRequest(encryptRequest){

}

EncryptionRequestProtoHelper::~EncryptionRequestProtoHelper() {

}

EncryptionRequestProtoHelper& EncryptionRequestProtoHelper::setAsn1Message(const std::vector<uint8_t>& bytes) {
    this->encryptRequest.set_asn1_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionRequestProtoHelper& EncryptionRequestProtoHelper::operator = (const std::vector<uint8_t>& bytes) {
    this->encryptRequest.set_asn1_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionRequestProtoHelper& EncryptionRequestProtoHelper::setAsn1Message(const keto::crypto::SecureVector& bytes) {
    this->encryptRequest.set_asn1_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionRequestProtoHelper& EncryptionRequestProtoHelper::operator = (const keto::crypto::SecureVector& bytes) {
    this->encryptRequest.set_asn1_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionRequestProtoHelper::operator std::vector<uint8_t>() const {
    return keto::server_common::VectorUtils().copyStringToVector(this->encryptRequest.asn1_message());
}

EncryptionRequestProtoHelper::operator keto::crypto::SecureVector() const {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(this->encryptRequest.asn1_message());
}

EncryptionRequestProtoHelper::operator keto::proto::EncryptRequest() const {
    return this->encryptRequest;
}

}
}