/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusHelper.hpp
 * Author: ubuntu
 *
 * Created on June 16, 2018, 4:07 PM
 */

#ifndef MODULECONSENSUSHELPER_HPP
#define MODULECONSENSUSHELPER_HPP

#include <vector>
#include <memory>
#include <string>

#include "SoftwareConsensus.pb.h"

namespace keto {
namespace software_consensus {


class ModuleConsensusHelper {
public:
    ModuleConsensusHelper();
    ModuleConsensusHelper(const ModuleConsensusHelper& orig);
    virtual ~ModuleConsensusHelper();
    
    ModuleConsensusHelper& setSecret(const std::vector<uint8_t>& secret);
    ModuleConsensusHelper& setSeedHash(const std::vector<uint8_t>& seedHash);
    ModuleConsensusHelper& setSeedHash(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getSeedHash();
    ModuleConsensusHelper& setModuleHash(const std::vector<uint8_t>& moduleHash);
    ModuleConsensusHelper& setModuleHash(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getModuleHash();
    
    
private:
    keto::proto::ModuleConsensusMessage moduleConsensusMessage;
    
    
};


}
}

#endif /* MODULECONSENSUSHELPER_HPP */

