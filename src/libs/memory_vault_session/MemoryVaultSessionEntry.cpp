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

MemoryVaultSessionEntry::MemoryVaultSessionEntry(MemoryVaultSession* memoryVaultSession, const keto::crypto::SecureVector& entryId) :
                        valueIsSet(false), memoryVaultSession(memoryVaultSession), entryId(entryId) {
}

MemoryVaultSessionEntry::MemoryVaultSessionEntry(MemoryVaultSession* memoryVaultSession, const keto::crypto::SecureVector& entryId, const keto::crypto::SecureVector& value) :
        valueIsSet(false), memoryVaultSession(memoryVaultSession), entryId(entryId)   {
    setValue(value);
}

MemoryVaultSessionEntry::~MemoryVaultSessionEntry() {
    try {
        if (valueIsSet) {
            keto::proto::MemoryVaultRemoveEntryRequest request;
            request.set_vault(memoryVaultSession->getVaultName());
            request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(memoryVaultSession->generatePassword()));
            request.set_entry_id(keto::crypto::SecureVectorUtils().copySecureToString(this->entryId));

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

    //auto start = std::chrono::steady_clock::now();

    //std::cout << "[MemoryVaultSessionEntry::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "]setup to make call" << std::endl;
    keto::proto::MemoryVaultGetEntryRequest request;
    request.set_vault(this->memoryVaultSession->getVaultName());
    request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(
            this->memoryVaultSession->generatePassword()));
    request.set_entry_id(keto::crypto::SecureVectorUtils().copySecureToString(this->entryId));

    //std::cout << "[MemoryVaultSessionEntry::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] make call" << std::endl;
    keto::proto::MemoryVaultGetEntryResponse response =
            keto::server_common::fromEvent<keto::proto::MemoryVaultGetEntryResponse>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::MemoryVaultGetEntryRequest>(
                                    keto::server_common::Events::MEMORY_VAULT::GET_ENTRY,request)));

    //std::cout << "[MemoryVaultSessionEntry::getValue][" <<
    //          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "] return the value" << std::endl;
    return keto::crypto::SecureVectorUtils().copyStringToSecure(response.value());
}

void MemoryVaultSessionEntry::setValue(const keto::crypto::SecureVector& value) {

    keto::proto::MemoryVaultSetEntryRequest request;
    request.set_vault(this->memoryVaultSession->getVaultName());
    request.set_password(keto::crypto::SecureVectorUtils().copySecureToString(this->memoryVaultSession->generatePassword()));
    request.set_entry_id(keto::crypto::SecureVectorUtils().copySecureToString(this->entryId));
    request.set_value(keto::crypto::SecureVectorUtils().copySecureToString(value));

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