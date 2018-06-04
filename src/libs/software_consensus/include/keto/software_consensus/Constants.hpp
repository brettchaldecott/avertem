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

namespace keto {
namespace software_consensus {

class Constants {
public:
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    static const std::vector<std::string> EVENT_ORDER;
    
    static constexpr char* H_FILE_VERSION = "$Id$";
    static const char* CPP_FILE_VERSION;
private:

};


}
}


#endif /* CONSTANTS_HPP */

