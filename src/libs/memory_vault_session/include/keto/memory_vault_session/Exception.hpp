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

#ifndef KETO_MEMORYVAULTSESSION_EXCEPTION_HPP
#define KETO_MEMORYVAULTSESSION_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace memory_vault_session {
        
// the keto module exception base
KETO_DECLARE_EXCEPTION( MemoryVaultSessionException, "The memory vault session" );


// the keto module derived exception
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultSessionException, DuplicateMemoryVaultSessionException , "Duplicate Memory Vault Session exception.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultSessionException, UnknownMemoryVaultSessionException , "Unknown Memory Vault Session exception.");
KETO_DECLARE_DERIVED_EXCEPTION (MemoryVaultSessionException, PasswordCacheNotIntialized , "Password cache not initialized.");

}
}


#endif /* EXCEPTION_HPP */

