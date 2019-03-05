//
// Created by Brett Chaldecott on 2019/03/05.
//

#include "keto/key_store_utils/EncryptionResponseProtoHelper.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {


std::string EncryptionResponseProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EncryptionResponseProtoHelper::EncryptionResponseProtoHelper() {
    this->encryptResponse.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

EncryptionResponseProtoHelper::EncryptionResponseProtoHelper(const std::string& value) {
    this->encryptResponse.ParseFromString(value);
}

EncryptionResponseProtoHelper::EncryptionResponseProtoHelper(const keto::proto::EncryptResponse encryptResponse) : encryptResponse(encryptResponse){

}

EncryptionResponseProtoHelper::~EncryptionResponseProtoHelper() {

}

EncryptionResponseProtoHelper& EncryptionResponseProtoHelper::setAsn1Message(const std::vector<uint8_t>& bytes) {
    this->encryptResponse.set_asn1_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionResponseProtoHelper& EncryptionResponseProtoHelper::operator = (const std::vector<uint8_t>& bytes) {
    this->encryptResponse.set_asn1_message(keto::server_common::VectorUtils().copyVectorToString(bytes));
    return *this;
}

EncryptionResponseProtoHelper& EncryptionResponseProtoHelper::setAsn1Message(const keto::crypto::SecureVector& bytes) {
    this->encryptResponse.set_asn1_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionResponseProtoHelper& EncryptionResponseProtoHelper::operator = (const keto::crypto::SecureVector& bytes) {
    this->encryptResponse.set_asn1_message(keto::crypto::SecureVectorUtils().copySecureToString(bytes));
    return *this;
}

EncryptionResponseProtoHelper::operator std::string() const {
    return this->encryptResponse.asn1_message();
}

EncryptionResponseProtoHelper::operator std::vector<uint8_t>() const {
    return keto::server_common::VectorUtils().copyStringToVector(this->encryptResponse.asn1_message());
}

EncryptionResponseProtoHelper::operator keto::crypto::SecureVector() const {
    return keto::crypto::SecureVectorUtils().copyStringToSecure(this->encryptResponse.asn1_message());
}

EncryptionResponseProtoHelper::operator keto::proto::EncryptResponse() const {
    return this->encryptResponse;
}

}
}