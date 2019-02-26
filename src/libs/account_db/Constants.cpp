/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include "keto/account_db/Constants.hpp"


namespace keto {
namespace account_db {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
const char* Constants::ACCOUNTS_MAPPING = "accounts_mapping";
const char* Constants::BASE_GRAPH = "base_graph";
const char* Constants::GRAPH_BASE_DIR = "graph_base_dir";
const char* Constants::BASE_MASTER_ACCOUNT_HASH = "D594F22DC389E38B3DE7FA5630DBD9DCA16DA8A77097516FD37F9E25C6BE24D2";
    

const std::vector<std::string> Constants::DB_LIST = 
    {ACCOUNTS_MAPPING};
    
}
}

