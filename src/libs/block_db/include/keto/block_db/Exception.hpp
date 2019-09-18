/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 21, 2018, 8:16 AM
 */

#ifndef KETO_DB_EXCEPTION_HPP
#define KETO_DB_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"

namespace keto {
namespace block_db {

// the keto db
KETO_DECLARE_EXCEPTION( DBException, "DB Exception." );

KETO_DECLARE_DERIVED_EXCEPTION (DBException, DBConnectionException , "Failed to connect to the database.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, InvalidDBNameException , "The db name supplied is not configured.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, ZeroLengthHashListException , "The merkel tree is zero length.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, FailedToAddTheTransactionException , "Failed to add the signed transaction.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, FailedToAddTheChangeSetException , "Failed to add the change set.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, SignedChangeSetReleasedException , "The change set has been released.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, SignedBlockReleasedException , "The signed block has been released.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, SignedBlockFailureException , "Failed to sign a block.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, InvalidParentHashIdentifierException , "The parent hash identifier is invalid.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, InvalidLastBlockHashException , "The hash could not be found.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, InvalidTransactionHashException , "The hash is invalid.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, ChainNotInitializedException , "Chain not initialized exception.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, MasterTangleNotConfiguredException , "Master tangle not configured.");
KETO_DECLARE_DERIVED_EXCEPTION (DBException, LastTangleNotConfiguredException , "Master tangle not configured.");

    
}
}


#endif /* EXCEPTION_HPP */

