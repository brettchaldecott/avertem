//
// Created by Brett Chaldecott on 2019/03/05.
//

#include "keto/keystore/EncryptionService.hpp"
#include "KeyStore.pb.h"
#include "keto/key_store_utils/EncryptionResponseProtoHelper.hpp"
#include "keto/key_store_utils/EncryptionRequestProtoHelper.hpp"
#include "keto/keystore/KeyStoreWrapIndexManager.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/server_common/EventUtils.hpp"


namespace keto {
namespace keystore {

static EncryptionServicePtr singleton;

std::string EncryptionService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EncryptionService::EncryptionService() {

}

EncryptionService::~EncryptionService() {

}

EncryptionServicePtr EncryptionService::init() {
    return singleton = EncryptionServicePtr(new EncryptionService());
}

void EncryptionService::fin() {
    singleton.reset();
}

EncryptionServicePtr EncryptionService::getInstance() {
    return singleton;
}

keto::event::Event EncryptionService::encryptAsn1(const keto::event::Event& event) {
    keto::key_store_utils::EncryptionRequestProtoHelper encryptionRequestProtoHelper(
            keto::server_common::fromEvent<keto::proto::EncryptRequest>(event));
    keto::key_store_utils::EncryptionResponseProtoHelper encryptionResponseProtoHelper;
    encryptionResponseProtoHelper = KeyStoreWrapIndexManager::getInstance()->getEncryptor()->encrypt(
            encryptionRequestProtoHelper);

    return keto::server_common::toEvent<keto::proto::EncryptResponse>(encryptionResponseProtoHelper);
}

keto::event::Event EncryptionService::decryptAsn1(const keto::event::Event& event) {
    keto::key_store_utils::EncryptionRequestProtoHelper encryptionRequestProtoHelper(
            keto::server_common::fromEvent<keto::proto::EncryptRequest>(event));
    keto::key_store_utils::EncryptionResponseProtoHelper encryptionResponseProtoHelper;
    encryptionResponseProtoHelper = KeyStoreWrapIndexManager::getInstance()->getDecryptor()->decrypt(
            encryptionRequestProtoHelper);

    return keto::server_common::toEvent<keto::proto::EncryptResponse>(encryptionResponseProtoHelper);
}


}
}