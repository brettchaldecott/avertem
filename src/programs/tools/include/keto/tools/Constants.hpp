/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on February 13, 2018, 7:28 AM
 */

#ifndef KETO_TOOLS_CONSTANTS_HPP
#define KETO_TOOLS_CONSTANTS_HPP

namespace keto {
namespace tools {


class Constants {
public:
    // json keys
    static const char* SECRET_KEY;
    static const char* ENCODED_KEY;
    
    // commands
    static constexpr const char* GENERATE = "generate";
    static constexpr const char* ENCRYPT = "encrypt";
    static constexpr const char* DECRYPT = "decrypt";
    static constexpr const char* HASH = "hash";
    static constexpr const char* KEY = "key";
    
    // arguments
    static constexpr const char* KEYS = "keys";
    static constexpr const char* SOURCE = "source";
    static constexpr const char* OUTPUT = "output";
    static constexpr const char* INPUT = "input";
    
};



}
}

#endif /* CONSTANTS_HPP */

