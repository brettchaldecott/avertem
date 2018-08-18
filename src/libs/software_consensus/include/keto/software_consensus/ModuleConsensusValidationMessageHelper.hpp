/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleConsensusValidationMessageHelper.hpp
 * Author: ubuntu
 *
 * Created on August 17, 2018, 8:39 AM
 */

#ifndef MODULECONSENSUSVALIDATIONMESSAGEHELPER_HPP
#define MODULECONSENSUSVALIDATIONMESSAGEHELPER_HPP

#include "SoftwareConsensus.pb.h"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace software_consensus {

class ModuleConsensusValidationMessageHelper {
public:
    ModuleConsensusValidationMessageHelper();
    ModuleConsensusValidationMessageHelper(
            const keto::proto::ModuleConsensusValidationMessage& moduleConsensusValidationMessage);
    ModuleConsensusValidationMessageHelper(const ModuleConsensusValidationMessageHelper& orig) = default;
    virtual ~ModuleConsensusValidationMessageHelper();
    
    ModuleConsensusValidationMessageHelper& setValid(bool valid);
    bool isValid();
    
    operator keto::proto::ModuleConsensusValidationMessage();
    
private:
    keto::proto::ModuleConsensusValidationMessage moduleConsensusValidationMessage;
};


}
}

#endif /* MODULECONSENSUSVALIDATIONMESSAGEHELPER_HPP */

