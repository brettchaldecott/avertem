/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 16, 2018, 11:06 AM
 */

#ifndef KETO_MEMORYVAULT_EXCEPTION_HPP
#define KETO_MEMORYVAULT_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace memory_vault {
        
// the keto module exception base
KETO_DECLARE_EXCEPTION( MemoryVaultException, "An exception occurred in the memory vault" );


// the keto module derived exception
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, VaultEntryNotFoundException , "Failed to find the entry.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, DecryptionFailedException , "Failed to decrypt the entry.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, InvalidPasswordException , "Invalid password exception.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, InvalidSesssionException , "Invalid password exception.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, DuplicateVaultException , "Invalid password exception.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, UnknownVaultException , "Invalid password exception.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultException, InvalidCipherIDException , "Invalid cipher id exception.");
}
}


#endif /* EXCEPTION_HPP */

