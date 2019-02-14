/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include "keto/block_db/Constants.hpp"


namespace keto {
namespace block_db {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

const char* Constants::BLOCK_META_INDEX = "block_meta_index";
const char* Constants::BLOCKS_INDEX = "blocks";
const char* Constants::TRANSACTIONS_INDEX = "transactions";
const char* Constants::ACCOUNTS_INDEX = "accounts";
const char* Constants::CHILD_INDEX = "childs";

// is the initial block
const char* Constants::MASTER_CHAIN_KEY = "C15473DF3116F8FB62A8FE7D8333D770425CB3445FDC6E34F4627A2527972620";
const keto::asn1::HashHelper Constants::MASTER_CHAIN_HASH(MASTER_CHAIN_KEY,keto::common::StringEncoding::HEX);

const char* Constants::GENESIS_KEY     = "22aec58889504ab835d6fc62b79cd342cf13a6202883dd013781005b49e59df2";
const keto::asn1::HashHelper Constants::GENESIS_HASH(GENESIS_KEY,keto::common::StringEncoding::HEX);

// the parent key
const char* Constants::PARENT_KEY      = "PARENT_KEY";
const char* Constants::BLOCK_COUNT     = "BLOCK_COUNT";

const std::vector<std::string> Constants::DB_LIST = 
    {Constants::BLOCK_META_INDEX, Constants::BLOCKS_INDEX, Constants::TRANSACTIONS_INDEX, Constants::ACCOUNTS_INDEX,
    Constants::CHILD_INDEX};
    
}
}

