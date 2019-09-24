//
// Created by Brett Chaldecott on 2019/04/24.
//

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/block_db/SignedBlockWrapperMessageProtoHelper.hpp"
#include "keto/key_store_utils/EncryptionNetworkRequestProtoHelper.hpp"
#include "keto/key_store_utils/EncryptionNetworkResponseProtoHelper.hpp"

namespace keto {
namespace block_db {

std::string SignedBlockWrapperMessageProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignedBlockWrapperMessageProtoHelper::SignedBlockWrapperMessageProtoHelper(const SignedBlockWrapperProtoHelper& signedBlockWrapperProtoHelper) {
    std::string data = signedBlockWrapperProtoHelper;
    keto::key_store_utils::EncryptionNetworkRequestProtoHelper request(keto::server_common::VectorUtils().copyStringToVector(data));
    keto::key_store_utils::EncryptionNetworkResponseProtoHelper response(
            keto::server_common::fromEvent<keto::proto::EncryptNetworkResponse>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::EncryptNetworkRequest>(
                            keto::server_common::Events::ENCRYPT_NETWORK_BYTES::ENCRYPT,request))));
    std::vector<uint8_t> bytes = response;
    this->signedBlockWrapperMessage.set_encrypted_bytes(bytes.data(),bytes.size());
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

SignedBlockWrapperMessageProtoHelper::operator SignedBlockWrapperProtoHelperPtr() {
    return getSignedBlockWrapper();
}

SignedBlockWrapperProtoHelperPtr SignedBlockWrapperMessageProtoHelper::getSignedBlockWrapper() {
    keto::key_store_utils::EncryptionNetworkRequestProtoHelper request(keto::server_common::VectorUtils().copyStringToVector(
            this->signedBlockWrapperMessage.encrypted_bytes()));
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


}
}