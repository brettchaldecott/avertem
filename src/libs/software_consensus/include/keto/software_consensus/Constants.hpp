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

    static const std::vector<std::string> CONSENSUS_SESSION_ACCEPTED;

    static const std::vector<std::string> CONSENSUS_SESSION_CHECK;

    static const std::vector<std::string> CONSENSUS_HEARTBEAT;

    static const std::vector<std::string> CONSENSUS_CONFIMATION_HEARTBEAT;

    static const std::vector<std::string> CONSENSUS_SESSION_STATE;

    // protocol configuration
    static const char* NETWORK_PROTOCOL_DELAY_CONFIGURATION;
    static const int NETWORK_PROTOCOL_DELAY_DEFAULT;
    static const char* NETWORK_PROTOCOL_COUNT_CONFIGURATION;
    static const int NETWORK_PROTOCOL_COUNT_DEFAULT;
    
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

private:

};


}
}


#endif /* CONSTANTS_HPP */

