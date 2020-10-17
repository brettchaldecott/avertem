/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 14, 2018, 7:15 AM
 */

#ifndef CONSENSUS_MODULE_EXCEPTION_HPP
#define CONSENSUS_MODULE_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace consensus_module {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( ConsensusModuleException, "Consensus Module Exception." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (ConsensusModuleException, PublicKeyNotConfiguredException , "The public key was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (ConsensusModuleException, PrivateKeyNotConfiguredException , "The private key was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (ConsensusModuleException, SessionNotAccepted , "Current session is not accepted.");
    
}
}


#endif /* EXCEPTION_HPP */

