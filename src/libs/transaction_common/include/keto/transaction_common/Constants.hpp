/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 11, 2018, 9:47 AM
 */

#ifndef TRANSACTION_CONSTANTS_HPP
#define TRANSACTION_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace transaction_common {


class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static constexpr const long DEFAULT_EXIRY_DURATION = 60*60;

private:
};

}
}
#endif /* CONSTANTS_HPP */

