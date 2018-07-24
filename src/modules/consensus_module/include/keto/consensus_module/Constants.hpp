/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 3:36 PM
 */

#ifndef CONSENSUS_MODULE_CONSTANTS_HPP
#define CONSENSUS_MODULE_CONSTANTS_HPP

namespace keto {
namespace consensus_module {


class Constants {
public:
    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    // keys for server
    static constexpr const char* PRIVATE_KEY    = "server-private-key";
    static constexpr const char* PUBLIC_KEY     = "server-public-key";
};

}
}

#endif /* CONSTANTS_HPP */
