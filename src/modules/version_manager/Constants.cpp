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

#include "keto/version_manager/Constants.hpp"

namespace keto {
namespace version_manager {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

const char* Constants::CHECK_SCRIPT = "check_script";
const char* Constants::AUTO_UPDATE = "auto_update";


}
}
