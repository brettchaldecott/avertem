//
// Created by Brett Chaldecott on 2019/01/16.
//

#include "keto/memory_vault_module/MemoryVaultModuleService.hpp"

#include <condition_variable>

#include "HandShake.pb.h"
#include "MemoryVault.pb.h"

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/memory_vault/MemoryVaultManager.hpp"



namespace keto {
namespace memory_vault_module {

static MemoryVaultModuleServicePtr singleton;

std::string MemoryVaultModuleService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

MemoryVaultModuleService::MemoryVaultModuleService() {
}

MemoryVaultModuleService::~MemoryVaultModuleService() {

}

MemoryVaultModuleServicePtr MemoryVaultModuleService::init() {
    return singleton = MemoryVaultModuleServicePtr(new MemoryVaultModuleService());
}

MemoryVaultModuleServicePtr MemoryVaultModuleService::getInstance() {
    return singleton;
}

void MemoryVaultModuleService::fin() {
    singleton.reset();
}

keto::event::Event MemoryVaultModuleService::createVault(const keto::event::Event &event) {
    keto::proto::MemoryVaultCreate memoryVaultCreate =
            keto::server_common::fromEvent<keto::proto::MemoryVaultCreate>(event);


    keto::memory_vault::MemoryVaultManager::getInstance()->createVault(memoryVaultCreate.vault(),
            keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultCreate.session()),
            keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultCreate.password()));


    return event;
}

keto::event::Event MemoryVaultModuleService::addEntry(const keto::event::Event &event) {

    keto::proto::MemoryVaultAddEntryRequest memoryVaultAddEntryRequest =
            keto::server_common::fromEvent<keto::proto::MemoryVaultAddEntryRequest>(event);

    keto::memory_vault::MemoryVaultPtr memoryVaultPtr =
            keto::memory_vault::MemoryVaultManager::getInstance()->getVault(
                    memoryVaultAddEntryRequest.vault(),
                    keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultAddEntryRequest.password()));

    memoryVaultPtr->setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultAddEntryRequest.entry_id()),
                             keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultAddEntryRequest.password()),
                             keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultAddEntryRequest.value()));

    keto::proto::MemoryVaultAddEntryResponse memoryVaultAddEntryResponse;
    memoryVaultAddEntryResponse.set_entry_id(memoryVaultAddEntryRequest.entry_id());
    memoryVaultAddEntryResponse.set_result("success");
    return keto::server_common::toEvent<keto::proto::MemoryVaultAddEntryResponse>(memoryVaultAddEntryResponse);
}

keto::event::Event MemoryVaultModuleService::setEntry(const keto::event::Event &event) {
    keto::proto::MemoryVaultSetEntryRequest memoryVaultSetEntryRequest =
            keto::server_common::fromEvent<keto::proto::MemoryVaultSetEntryRequest>(event);

    keto::memory_vault::MemoryVaultPtr memoryVaultPtr =
            keto::memory_vault::MemoryVaultManager::getInstance()->getVault(
                    memoryVaultSetEntryRequest.vault(),
                    keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultSetEntryRequest.password()));

    memoryVaultPtr->setValue(keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultSetEntryRequest.entry_id()),
                             keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultSetEntryRequest.password()),
                             keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultSetEntryRequest.value()));

    keto::proto::MemoryVaultSetEntryResponse memoryVaultSetEntryResponse;
    memoryVaultSetEntryResponse.set_entry_id(memoryVaultSetEntryRequest.entry_id());
    memoryVaultSetEntryResponse.set_result("success");
    return keto::server_common::toEvent<keto::proto::MemoryVaultSetEntryResponse>(memoryVaultSetEntryResponse);
}

keto::event::Event MemoryVaultModuleService::getEntry(const keto::event::Event &event) {
    keto::proto::MemoryVaultGetEntryRequest memoryVaultGetEntryRequest =
            keto::server_common::fromEvent<keto::proto::MemoryVaultGetEntryRequest>(event);

    keto::memory_vault::MemoryVaultPtr memoryVaultPtr =
            keto::memory_vault::MemoryVaultManager::getInstance()->getVault(
                    memoryVaultGetEntryRequest.vault(),
                    keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultGetEntryRequest.password()));


    keto::proto::MemoryVaultGetEntryResponse memoryVaultGetEntryResponse;
    memoryVaultGetEntryResponse.set_entry_id(memoryVaultGetEntryRequest.entry_id());
    memoryVaultGetEntryResponse.set_value(keto::crypto::SecureVectorUtils().copySecureToString(memoryVaultPtr->getValue(
            keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultGetEntryRequest.entry_id()),
            keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultGetEntryRequest.password()))));
    memoryVaultGetEntryResponse.set_result("success");
    return keto::server_common::toEvent<keto::proto::MemoryVaultGetEntryResponse>(memoryVaultGetEntryResponse);
}

keto::event::Event MemoryVaultModuleService::removeEntry(const keto::event::Event &event) {
    keto::proto::MemoryVaultRemoveEntryRequest memoryVaultRemoveEntryRequest =
            keto::server_common::fromEvent<keto::proto::MemoryVaultRemoveEntryRequest>(event);

    keto::memory_vault::MemoryVaultPtr memoryVaultPtr =
            keto::memory_vault::MemoryVaultManager::getInstance()->getVault(
                    memoryVaultRemoveEntryRequest.vault(),
                    keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultRemoveEntryRequest.password()));


    keto::proto::MemoryVaultRemoveEntryResponse memoryVaultRemoveEntryResponse;
    memoryVaultRemoveEntryResponse.set_entry_id(memoryVaultRemoveEntryRequest.entry_id());
    memoryVaultPtr->removeValue(
            keto::crypto::SecureVectorUtils().copyStringToSecure(memoryVaultRemoveEntryRequest.entry_id()));
    memoryVaultRemoveEntryResponse.set_result("success");
    return keto::server_common::toEvent<keto::proto::MemoryVaultRemoveEntryResponse>(memoryVaultRemoveEntryResponse);
}


}
}