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

#ifndef KETO_BLOCK_EXCEPTION_HPP
#define KETO_BLOCK_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace block {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( BlockException, "Block exception." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, PrivateKeyNotConfiguredException , "The server private key configured was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, FaucetNotConfiguredException , "The faucet account has not been configured.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, PublicKeyNotConfiguredException , "The server public key configured was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, BlockProducerTerminatedException , "The block producer has been terminated.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, NotBlockProducerException , "Not enabled as a block producer.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, BlockProducerNotAcceptedByNetworkException , "Block producer not accepted by network.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, NetworkFeeRatioNotSetException , "The master node network fee ratio was not set.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, UnsupportedTransactionStatusException , "The transaction status is unsupported.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, UnprocessedTransactionsException , "Unprocessed transactions in the pending queue.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, ElectionFailedException , "The election failed.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, UnsyncedStateCannotProvideDate , "This node is unsynced and cannot provide data.");
KETO_DECLARE_DERIVED_EXCEPTION (BlockException, ReRouteMessageException , "The message must be rerouted.");

    
}
}


#endif /* EXCEPTION_HPP */

