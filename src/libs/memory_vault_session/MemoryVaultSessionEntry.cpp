//
// Created by Brett Chaldecott on 2019/01/14.
//

#include <keto/crypto/SecureVectorUtils.hpp>
#include "keto/memory_vault_session/MemoryVaultSessionEntry.hpp"
#include "keto/memory_vault_session/MemoryVaultSession.hpp"


#include "MemoryVault.pb.h"

#include "keto/event/Event.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

namespace keto {
namespace memory_vault_session {

std::string MemoryVaultSessionEntry::getSourceVersion() {
    return OBFUSCATED("$Id:");
}

MemoryVaultSessionEntry::MemoryVaultSessionEntry(MemoryVaultSession* memoryVaultSession, const keto::crypto::SecureVector& entryId, const keto::memory_vault_session::PasswordCachePtr& passwordCachePtr) :
                        valueIsSet(false), memoryVaultSession(memoryVaultSession), entryId(entryId), passwordCachePtr(passwordCachePtr), slotId(memoryVaultSession->getSlot())  {
    KETO_LOG_INFO << "[MemoryVaultSessionEntry::MemoryVaultSessionEntry] Create the new session entry : " << this->slotId;
}

MemoryVaultSessionEntry::MemoryVaultSessionEntry(MemoryVaultSession* memoryVaultSession, const keto::crypto::SecureVector& entryId, const keto::crypto::SecureVector& value, const keto::memory_vault_session::PasswordCachePtr& passwordCachePtr) :
        valueIsSet(false), memoryVaultSession(memoryVaultSession), entryId(entryId), passwordCachePtr(passwordCachePtr), slotId(memoryVaultSession->getSlot())   {
    KETO_LOG_INFO << "[MemoryVaultSessionEntry::MemoryVaultSessionEntry] Create the new session entry : " << this->slotId;
    setValue(value);
}

MemoryVaultSessionEntry::~MemoryVaultSessionEntry() {
    try {
        KETO_LOG_INFO << "[MemoryVaultSessionEntry::~MemoryVaultSessionEntry] Remove the slot : " << this->slotId;
        if (valueIsSet) {
            keto::proto::MemoryVaultRemoveEntryRequest request;
            request.set_vault(memoryVaultSession->getVaultName());
            request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(memoryVaultSession->processPassword(passwordCachePtr)));
            request.set_entry_id(keto::crypto::SecureVectorUtils().copySecureToString(this->entryId));
            request.set_slot_id(this->slotId);

            keto::proto::MemoryVaultRemoveEntryResponse response =
                    keto::server_common::fromEvent<keto::proto::MemoryVaultRemoveEntryResponse>(
                            keto::server_common::processEvent(
                                    keto::server_common::toEvent<keto::proto::MemoryVaultRemoveEntryRequest>(
                                            keto::server_common::Events::MEMORY_VAULT::REMOVE_ENTRY, request)));
        }
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "Failed to remove the entry : " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "Failed to remove the entry : " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "Failed to remove the entry : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "Failed to remove the entry.";
    }
}

keto::crypto::SecureVector MemoryVaultSessionEntry::getValue() {

    keto::proto::MemoryVaultGetEntryRequest request;
    request.set_vault(this->memoryVaultSession->getVaultName());
    request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(
            this->memoryVaultSession->processPassword(passwordCachePtr)));
    request.set_entry_id(keto::crypto::SecureVectorUtils().copySecureToString(this->entryId));
    request.set_slot_id(this->slotId);

    keto::proto::MemoryVaultGetEntryResponse response =
            keto::server_common::fromEvent<keto::proto::MemoryVaultGetEntryResponse>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MemoryVaultGetEntryRequest>(
                                    keto::server_common::Events::MEMORY_VAULT::GET_ENTRY,request)));

    return keto::crypto::SecureVectorUtils().copyStringToSecure(response.value());
}

void MemoryVaultSessionEntry::setValue(const keto::crypto::SecureVector& value) {

    keto::proto::MemoryVaultSetEntryRequest request;
    request.set_vault(this->memoryVaultSession->getVaultName());
    request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(memoryVaultSession->processPassword(passwordCachePtr)));
    request.set_entry_id(keto::crypto::SecureVectorUtils().copySecureToString(this->entryId));
    request.set_value(keto::crypto::SecureVectorUtils().copySecureToString(value));
    request.set_slot_id(this->slotId);

    keto::proto::MemoryVaultSetEntryResponse response =
        keto::server_common::fromEvent<keto::proto::MemoryVaultSetEntryResponse>(
                keto::server_common::processEvent(
                        keto::server_common::toEvent<keto::proto::MemoryVaultSetEntryRequest>(
                                keto::server_common::Events::MEMORY_VAULT::SET_ENTRY,request)));

    valueIsSet = true;
}


keto::crypto::SecureVector MemoryVaultSessionEntry::getEntryId() {
    return this->entryId;
}

}
}
