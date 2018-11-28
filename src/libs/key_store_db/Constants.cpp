/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.cpp
 * Author: brettchaldecott
 * 
 * Created on 24 November 2018, 7:18 AM
 */

#include "keto/key_store_db/Constants.hpp"


namespace keto {
namespace key_store_db {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

// string constants
const char* Constants::KEY_STORE = "key_store";

const std::vector<std::string> Constants::DB_LIST = {KEY_STORE};



}
}
