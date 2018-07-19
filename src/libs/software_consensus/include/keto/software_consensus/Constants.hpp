/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on May 29, 2018, 11:13 AM
 */

#ifndef SOFTWARE_CONSENSUS_CONSTANTS_HPP
#define SOFTWARE_CONSENSUS_CONSTANTS_HPP

#include <vector>
#include <string>
#include <iostream>

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class Constants {
public:
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    static const std::vector<std::string> EVENT_ORDER;
    
    static const std::vector<std::string> CONSENSUS_SESSION_ORDER;
    
    inline static std::string getVersion() {
        return OBFUSCATED("$Id: cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $");
    };
    
    static std::string getSourceVersion();
    
    //static const MetaStringType CPP_FILE_VERSION;
private:

};


}
}


#endif /* CONSTANTS_HPP */

