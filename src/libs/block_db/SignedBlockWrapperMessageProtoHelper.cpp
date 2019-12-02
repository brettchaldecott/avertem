//
// Created by Brett Chaldecott on 2019/04/24.
//

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/block_db/SignedBlockWrapperMessageProtoHelper.hpp"

#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"

#include "keto/key_store_utils/EncryptionNetworkRequestProtoHelper.hpp"
#include "keto/key_store_utils/EncryptionNetworkResponseProtoHelper.hpp"

#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/server_common/StringUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace block_db {

std::string SignedBlockWrapperMessageProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignedBlockWrapperMessageProtoHelper::SignedBlockWrapperMessageProtoHelper(const keto::asn1::HashHelper& hash) {
    std::vector<uint8_t> bytes = keto::crypto::SecureVectorUtils().copyFromSecure(hash);
    keto::asn1::TimeHelper currentTime;
    UTCTime_t utcTime = currentTime;
    std::vector<uint8_t> timeBytes = keto::asn1::SerializationHelper<UTCTime_t>(&utcTime,&asn_DEF_UTCTime);
    bytes.insert(bytes.end(),timeBytes.begin(),timeBytes.end());

    this->signedBlockWrapperMessage.set_message_hash(
            keto::crypto::SecureVectorUtils().copySecureToString(keto::crypto::HashGenerator().generateHash(bytes)));
}

SignedBlockWrapperMessageProtoHelper::SignedBlockWrapperMessageProtoHelper(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage) :
    signedBlockWrapperMessage(signedBlockWrapperMessage){
}

SignedBlockWrapperMessageProtoHelper::SignedBlockWrapperMessageProtoHelper(const std::string& signedBlockWrapperMessage) {
    this->signedBlockWrapperMessage.ParseFromString(signedBlockWrapperMessage);
}

SignedBlockWrapperMessageProtoHelper::~SignedBlockWrapperMessageProtoHelper() {

}


SignedBlockWrapperMessageProtoHelper::operator std::string() {
    return this->signedBlockWrapperMessage.SerializeAsString();
}

SignedBlockWrapperMessageProtoHelper::operator keto::proto::SignedBlockWrapperMessage() {
    return this->signedBlockWrapperMessage;
}

SignedBlockWrapperProtoHelperPtr SignedBlockWrapperMessageProtoHelper::operator[] (int index) {
    return getSignedBlockWrapper(index);
}

SignedBlockWrapperMessageProtoHelper& SignedBlockWrapperMessageProtoHelper::addSignedBlockWrapper(
        const SignedBlockWrapperProtoHelperPtr& signedBlockWrapperProtoHelperPtr) {
    std::string data = *signedBlockWrapperProtoHelperPtr;
    keto::key_store_utils::EncryptionNetworkRequestProtoHelper request(keto::server_common::VectorUtils().copyStringToVector(data));
    keto::key_store_utils::EncryptionNetworkResponseProtoHelper response(
            keto::server_common::fromEvent<keto::proto::EncryptNetworkResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptNetworkRequest>(
                            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::ENCRYPT,request))));
    std::vector<uint8_t> bytes = response;
    *this->signedBlockWrapperMessage.add_encrypted_bytes() = std::string(bytes.begin(),bytes.end());
    return *this;
}

SignedBlockWrapperMessageProtoHelper& SignedBlockWrapperMessageProtoHelper::addSignedBlockWrapper(
            const SignedBlockWrapperProtoHelper& signedBlockWrapperProtoHelper) {
    std::string data = signedBlockWrapperProtoHelper;
    keto::key_store_utils::EncryptionNetworkRequestProtoHelper request(keto::server_common::VectorUtils().copyStringToVector(data));
    keto::key_store_utils::EncryptionNetworkResponseProtoHelper response(
            keto::server_common::fromEvent<keto::proto::EncryptNetworkResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptNetworkRequest>(
                            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::ENCRYPT,request))));
    std::vector<uint8_t> bytes = response;
    *this->signedBlockWrapperMessage.add_encrypted_bytes() = std::string(bytes.begin(),bytes.end());
    return *this;
}

int SignedBlockWrapperMessageProtoHelper::size() const {
    return this->signedBlockWrapperMessage.encrypted_bytes_size();
}

std::vector<SignedBlockWrapperProtoHelperPtr> SignedBlockWrapperMessageProtoHelper::getSignedBlockWrappers() const {
    std::vector<SignedBlockWrapperProtoHelperPtr> result;
    for (int index = 0; index < size(); index++) {
        result.push_back(getSignedBlockWrapper(index));
    }
    return result;
}

SignedBlockWrapperProtoHelperPtr SignedBlockWrapperMessageProtoHelper::getSignedBlockWrapper(int index) const {
    keto::key_store_utils::EncryptionNetworkRequestProtoHelper request(keto::server_common::VectorUtils().copyStringToVector(
            this->signedBlockWrapperMessage.encrypted_bytes(index)));
    keto::key_store_utils::EncryptionNetworkResponseProtoHelper response(
            keto::server_common::fromEvent<keto::proto::EncryptNetworkResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptNetworkRequest>(
                            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::DECRYPT,request))));
    std::vector<uint8_t> bytes = response;
    return SignedBlockWrapperProtoHelperPtr(
            new SignedBlockWrapperProtoHelper(
                    keto::server_common::VectorUtils().copyVectorToString(bytes)
            ));
}


SignedBlockWrapperMessageProtoHelper& SignedBlockWrapperMessageProtoHelper::setProducerEnding(bool producerEnding) {
    this->signedBlockWrapperMessage.set_producer_ending(producerEnding);
    return *this;
}

bool SignedBlockWrapperMessageProtoHelper::getProducerEnding() {
    return this->signedBlockWrapperMessage.producer_ending();
}

SignedBlockWrapperMessageProtoHelper& SignedBlockWrapperMessageProtoHelper::setMessageHash(const keto::asn1::HashHelper& hash) {
    this->signedBlockWrapperMessage.set_message_hash(hash);
    return *this;
}

keto::asn1::HashHelper SignedBlockWrapperMessageProtoHelper::getMessageHash() const {
    return keto::asn1::HashHelper(this->signedBlockWrapperMessage.message_hash());
}

std::vector<keto::asn1::HashHelper> SignedBlockWrapperMessageProtoHelper::getTangles() const {
    std::vector<keto::asn1::HashHelper> result;
    for (int index = 0; index < this->signedBlockWrapperMessage.tangles_size(); index++) {
        result.push_back(keto::asn1::HashHelper(this->signedBlockWrapperMessage.tangles(index)));
    }
    return result;
}

SignedBlockWrapperMessageProtoHelper& SignedBlockWrapperMessageProtoHelper::setTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    for (keto::asn1::HashHelper tangle : tangles) {
        *this->signedBlockWrapperMessage.add_tangles() = tangle;
    }
    return *this;
}

}
}