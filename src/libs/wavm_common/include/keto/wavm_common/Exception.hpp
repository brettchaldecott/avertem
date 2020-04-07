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

#ifndef KETO_WAVM_COMMON_EXCEPTION_HPP
#define KETO_WAVM_COMMON_EXCEPTION_HPP

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
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidSubjectForContract , "The subject being modified does not match the contract.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidContractBalance , "The contract balance is invalid.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, UnsupportedModelType , "The model type is not supported.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidActionIdForThisSession , "Invalid action id.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidNestedTransactionIdForThisSession , "Invalid nested transaction id.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidTransactionIdForThisSession , "Invalid action id.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, EmscriptInstanciateFailed , "Emscript instanciate failed.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, FailedToInstanciateModule , "Failed to instanciate the module.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ModuleMemoryInvalid , "Module memory is invalid.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, InvalidActionModelRequest , "Invalid action model request.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, TransactionAlreadySubmitted , "Transaction already submitted.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, FailedToParseFormMessage , "Failed to parse the form message.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ParentRequestException , "Parent requested resulted in an exception.");


}
}

#endif /* EXCEPTION_HPP */

