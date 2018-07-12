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

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"

namespace keto {
namespace software_consensus {


class ModuleConsensusHelper {
public:
    inline static std::string getVersion() {
        return OBFUSCATED("$Id: cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $");
    };
    
    static std::string getSourceVersion();

    
    ModuleConsensusHelper();
    ModuleConsensusHelper(const keto::proto::ModuleConsensusMessage& moduleConsensusMessage);
    ModuleConsensusHelper(const ModuleConsensusHelper& orig) = default;
    virtual ~ModuleConsensusHelper();
    
    ModuleConsensusHelper& setSeedHash(const std::vector<uint8_t>& seedHash);
    ModuleConsensusHelper& setSeedHash(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getSeedHash();
    ModuleConsensusHelper& setModuleHash(const std::vector<uint8_t>& moduleHash);
    ModuleConsensusHelper& setModuleHash(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getModuleHash();
    
    operator keto::proto::ModuleConsensusMessage();
    keto::proto::ModuleConsensusMessage getModuleConsensusMessage();
private:
    keto::proto::ModuleConsensusMessage moduleConsensusMessage;
    
    
};


}
}

#endif /* MODULECONSENSUSHELPER_HPP */

