/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 31, 2018, 4:37 PM
 */

#ifndef TRANSACTION_COMMON_EXCEPTION_HPP
#define TRANSACTION_COMMON_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace wavm_common {

// the keto environment exception base
KETO_DECLARE_EXCEPTION( WavmCommonException, "Wavm common failed." );

// the keto wavm derived exception
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidContractException , "Failed to parse the contract.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, LinkingFailedException , "Linking failed.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, MissingEntryPointException , "The entry point to the web assembly was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ContactExecutionFailedException , "Contract execution failed.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, UnsupportedDataTypeTransactionException , "Unsupported data type transactions.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, SparqlQueryFailed , "The sparql query failed.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, NodeNotFoundRDFException , "The rdf node was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, UnrecognisedTransactionStatusException , "The status of the transaction is not recognised.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, PrivateKeyNotFoundException , "The private key for this node was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, PublicKeyNotFoundException , "The public key for this node was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidTransactionReferenceException , "Invalid transaction reference exception.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, UnsupportedInvocationStatusException , "Unsupported invocation status.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, RdfIndexOutOfRangeException , "Unsupported invocation status.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ContractExecutionFailedException , "Contract execution failed.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ContractUnsupportedResultException , "Contract return an unsupported result.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidWavmSessionTypeException , "Trying to perform invalid operation on session type.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, RoleIndexOutOfBoundsException , "The role index is out of bounds.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ParameterIndexOutOfBoundsException , "The parameter index is out of bounds.");


}
}

#endif /* EXCEPTION_HPP */

