/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 16, 2018, 8:59 AM
 */

#ifndef KETO_SOFTWARE_CONSENSUS_EXCEPTION_HPP
#define KETO_SOFTWARE_CONSENSUS_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace software_consensus {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( SoftwareConsensusException, "Software Consensus Exeption." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (SoftwareConsensusException, FailedParseConsensusObjectException , "Failed to parse the consensus string object.");
KETO_DECLARE_DERIVED_EXCEPTION (SoftwareConsensusException, NoSoftwareHashConsensusException , "Failed to parse the consensus string object.");
KETO_DECLARE_DERIVED_EXCEPTION (SoftwareConsensusException, FailedToAddSystemHashException , "Failed to parse the consensus string object.");
KETO_DECLARE_DERIVED_EXCEPTION (SoftwareConsensusException, InvalidSessionException , "The session is invalid, could not setup.");
KETO_DECLARE_DERIVED_EXCEPTION (SoftwareConsensusException, ConsenusScriptExecutionException , "The consensus script execution failed.");

}
}

#endif /* EXCEPTION_HPP */

