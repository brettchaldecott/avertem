/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.cpp
 * Author: ubuntu
 * 
 * Created on March 8, 2018, 8:09 AM
 */

#include "keto/block/Constants.hpp"

namespace keto {
namespace block {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

const char* Constants::GENESIS_CONFIG   = "genesis_config";

// keys for server
const char* Constants::PRIVATE_KEY      = "server-private-key";
const char* Constants::PUBLIC_KEY       = "server-public-key";

const char* Constants::BLOCK_PRODUCER_ENABLED
                                        = "block_producer_enabled";
const char* Constants::BLOCK_PRODUCER_ENABLED_TRUE
                                        = "true";

const char* Constants::BLOCK_PRODUCER_SAFE_MODE
                                        = "block_producer_safe_mode";
const char* Constants::BLOCK_PRODUCER_SAFE_MODE_ENABLED_TRUE
                                        = "true";

const char* Constants::FAUCET_ACCOUNT   = "faucet_account";
const long Constants::BLOCK_PRODUCER_SAFE_MODE_DELAY
                                        = 10;


const char* Constants::NETWORK_FEE_RATIO = "network_fee_ratio";
const long Constants::MAX_RUN_TIME      = 60 * 10;
const long Constants::SYNC_RETRY_DELAY_MIN  = 60 * 9;



const long Constants::SYNC_EXPIRY_TIME  = 60;


// tangle configuration
const int Constants::MAX_TANGLE_ACCOUNTS = 50;
const int Constants::MAX_TANGLES_TO_ACCOUNT = 50;

// the delay values
const int Constants::ACTIVATE_PRODUCER_DELAY    = 45;
const int Constants::BLOCK_TIME                 = 15;
const int Constants::BLOCK_PRDUCER_DEACTIVATE_CHECK_DELAY    = 10;
const int Constants::BLOCK_PRODUCER_RETRY_MAX   = 4;
const int Constants::MAX_SIGNED_BLOCK_WRAPPER_CACHE_SIZE = 100;

const char* Constants::FaucetRequest::SUBJECT   = "http://keto-coin.io/schema/rdf/1.0/keto/Faucet#Faucet";
const char* Constants::FaucetRequest::ID        = "http://keto-coin.io/schema/rdf/1.0/keto/Faucet#id";
const char* Constants::FaucetRequest::ACCOUNT   = "http://keto-coin.io/schema/rdf/1.0/keto/Faucet#account";
const char* Constants::FaucetRequest::DATE_TIME = "http://keto-coin.io/schema/rdf/1.0/keto/Faucet#dateTime";
const char* Constants::FaucetRequest::TANGLES   = "http://keto-coin.io/schema/rdf/1.0/keto/Faucet#tangles";
const char* Constants::FaucetRequest::PROOF     = "http://keto-coin.io/schema/rdf/1.0/keto/Faucet#proof";


const char* Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION = "avertem__base_account_transaction";
const char* Constants::SYSTEM_CONTRACT::FEE_PAYMENT = "avertem__fee_payment";
const char* Constants::SYSTEM_CONTRACT::NESTED_TRANSACTION = "avertem__nested_transaction";
const char* Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION = "avertem__faucet_transaction";


const std::vector<const char*> Constants::SYSTEM_CONTRACTS{
        Constants::SYSTEM_CONTRACT::BASE_ACCOUNT_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FEE_PAYMENT,
        Constants::SYSTEM_CONTRACT::NESTED_TRANSACTION,
        Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION
};


}
}
