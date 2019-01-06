/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on January 16, 2018, 12:08 PM
 */

#ifndef KETO_MEMORYVAULT_CONSTANTS_HPP
#define KETO_MEMORYVAULT_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace memory_vault {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static constexpr size_t BASE_SIZE = 6;
    static constexpr size_t MODULAS_SIZE = 4;
    static constexpr size_t ITERATION_BITS_SIZE = 16;
    
private:
};
    
}
}


#endif /* CONSTANTS_HPP */

