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

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ModuleConsensusGenerator {
public:
    static std::string getVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    
    ModuleConsensusGenerator(
            const keto::proto::ModuleConsensusMessage& moduleConsensusMessage);
    ModuleConsensusGenerator(const ModuleConsensusGenerator& orig) = default;
    virtual ~ModuleConsensusGenerator();
    
    keto::proto::ModuleConsensusMessage generate();
    
private:
    keto::proto::ModuleConsensusMessage softwareConsensusMessage;
};


}
}


#endif /* MODULECONSENSUSGENERATOR_HPP */

