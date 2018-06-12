/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusInterface.hpp
 * Author: ubuntu
 *
 * Created on May 31, 2018, 10:55 AM
 */

#ifndef MODULECONSENSUSINTERFACE_HPP
#define MODULECONSENSUSINTERFACE_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace software_consensus {

class ModuleConsensusInterface {
public:
    
    static std::string getVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    virtual keto::crypto::SecureVector generateModuleHash(
            const keto::crypto::SecureVector& salt);
    
};

}
}



#endif /* MODULECONSENSUSINTERFACE_HPP */

