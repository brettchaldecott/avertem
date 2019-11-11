/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on March 8, 2018, 8:09 AM
 */

#ifndef BLOCK_MODULE_CONSTANTS_HPP
#define BLOCK_MODULE_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace block {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    // string constants
    static const char* GENESIS_CONFIG;
    
    // keys for server
    static const char* PRIVATE_KEY;
    static const char* PUBLIC_KEY;
    
    // block producer
    static const char* BLOCK_PRODUCER_ENABLED;
    static const char* BLOCK_PRODUCER_ENABLED_TRUE;
    static const char* BLOCK_PRODUCER_SAFE_MODE;
    static const char* BLOCK_PRODUCER_SAFE_MODE_ENABLED_TRUE;
    static const char* FAUCET_ACCOUNT;
    static const long BLOCK_PRODUCER_SAFE_MODE_DELAY;

    // network fee manager
    static const char* NETWORK_FEE_RATIO;
    static const long MAX_RUN_TIME;
    static const long SYNC_RETRY_DELAY_MIN;

    // max time
    static const long SYNC_EXPIRY_TIME;


    // state storage
    static constexpr const char* STATE_STORAGE_CONFIG = "block_state_storage";
    static constexpr const char* STATE_STORAGE_DEFAULT = "data/block/state.ini";

    static constexpr const char* PERSISTED_STATE = "BlockProducer.state";

    // tangle configuration
    static const int MAX_TANGLE_ACCOUNTS;
    static const int MAX_TANGLES_TO_ACCOUNT;

    // the delay values
    static const int ACTIVATE_PRODUCER_DELAY;
    static const int BLOCK_TIME;
    static const int BLOCK_PRDUCER_DEACTIVATE_CHECK_DELAY;
    static const int BLOCK_PRODUCER_RETRY_MAX;


    class FaucetRequest {
    public:
        static const char* SUBJECT;
        static const char* ID;
        static const char* ACCOUNT;
        static const char* DATE_TIME;
        static const char* TANGLES;
        static const char* PROOF;


    };


    class SYSTEM_CONTRACT {
    public:
        static const char* BASE_ACCOUNT_TRANSACTION;
        static const char* FEE_PAYMENT;
        static const char* NESTED_TRANSACTION;
        static const char* FAUCET_TRANSACTION;
    };
    static const std::vector<const char*> SYSTEM_CONTRACTS;

};


}
}

#endif /* CONSTANTS_HPP */

