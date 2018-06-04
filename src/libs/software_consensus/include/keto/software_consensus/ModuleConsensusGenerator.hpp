/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusGenerator.hpp
 * Author: ubuntu
 *
 * Created on May 31, 2018, 10:14 AM
 */

#ifndef MODULECONSENSUSGENERATOR_HPP
#define MODULECONSENSUSGENERATOR_HPP

#include "keto/software_consensus/ModuleConsensusInterface.hpp"

namespace keto {
namespace software_consensus {

class ModuleConsensusGenerator {
public:
    ModuleConsensusGenerator(const keto::proto::SoftwareConsensusMessage& 
            softwareConsensusMessage);
    ModuleConsensusGenerator(const ModuleConsensusGenerator& orig) = default;
    virtual ~ModuleConsensusGenerator();
    
    keto::proto::SoftwareConsensusMessage generate(
            const ModuleConsensusInterface& moduleConsensusInterface);
    
private:
    keto::proto::SoftwareConsensusMessage softwareConsensusMessage;
};


}
}


#endif /* MODULECONSENSUSGENERATOR_HPP */

