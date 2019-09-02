/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on February 16, 2018, 8:18 AM
 */

#ifndef ELECTION_COMMON_CONSTANTS_HPP
#define ELECTION_COMMON_CONSTANTS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace election_common {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;

    static const std::vector<std::string> ELECTION_INTERNAL_PUBLISH;
    static const std::vector<std::string> ELECTION_PROCESS_PUBLISH;
    static const std::vector<std::string> ELECTION_PROCESS_CONFIRMATION;

private:

};


}
}

#endif /* CONSTANTS_HPP */

