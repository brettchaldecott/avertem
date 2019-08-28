/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.cpp
 * Author: ubuntu
 * 
 * Created on February 16, 2018, 8:18 AM
 */

#include "keto/election_common/Constants.hpp"


namespace keto {
namespace election_common {

std::string Constants::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


}
}