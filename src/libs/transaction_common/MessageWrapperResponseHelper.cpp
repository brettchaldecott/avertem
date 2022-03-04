//
// Created by Brett Chaldecott on 2022/02/23.
//

#include "include/keto/transaction_common/MessageWrapperResponseHelper.hpp"

#include <botan/hex.h>
#include <botan/base64.h>

#include "keto/crypto/SecureVectorUtils.hpp"

namespace keto {
namespace transaction_common {

std::string MessageWrapperResponseHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

MessageWrapperResponseHelper::MessageWrapperResponseHelper() {

}

MessageWrapperResponseHelper::MessageWrapperResponseHelper(
        const keto::proto::MessageWrapperResponse& messageWrapperResponse) :
        messageWrapperResponse(messageWrapperResponse) {

}

MessageWrapperResponseHelper::MessageWrapperResponseHelper(const std::string& msg) {
    this->messageWrapperResponse.ParseFromString(msg);
}

MessageWrapperResponseHelper::~MessageWrapperResponseHelper() {

}

bool MessageWrapperResponseHelper::isSuccess() {
    return this->messageWrapperResponse.success();
}

MessageWrapperResponseHelper& MessageWrapperResponseHelper::setSuccess(bool success) {
    this->messageWrapperResponse.set_success(success);
    return *this;
}

std::string MessageWrapperResponseHelper::getResult() {
    return this->messageWrapperResponse.result();
}

MessageWrapperResponseHelper& MessageWrapperResponseHelper::setResult(const std::string& result) {
    this->messageWrapperResponse.set_result(result);
    return *this;
}

std::string MessageWrapperResponseHelper::getMsg() {
    return this->messageWrapperResponse.msg();
}

std::string MessageWrapperResponseHelper::getBinaryMsg() {
    keto::crypto::SecureVector binaryValue = Botan::hex_decode_locked(this->messageWrapperResponse.msg(),true);
    return keto::crypto::SecureVectorUtils().copySecureToString(binaryValue);
}

MessageWrapperResponseHelper& MessageWrapperResponseHelper::setMsg(const std::string& msg) {
    this->messageWrapperResponse.set_msg(msg);
    return *this;
}

MessageWrapperResponseHelper& MessageWrapperResponseHelper::setBinaryMsg(const std::string& binaryMsg) {
    this->messageWrapperResponse.set_msg(
            Botan::hex_encode(keto::crypto::SecureVectorUtils().copyStringToSecure(binaryMsg),true));
    return *this;
}

MessageWrapperResponseHelper::operator std::string() {
    return this->messageWrapperResponse.SerializeAsString();
}

MessageWrapperResponseHelper::operator keto::proto::MessageWrapperResponse() {
    return this->messageWrapperResponse;
}

keto::proto::MessageWrapperResponse MessageWrapperResponseHelper::getMessageWrapperResponse() {
    return this->messageWrapperResponse;
}



}
}