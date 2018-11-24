/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleSessionMessageHelper.hpp
 * Author: ubuntu
 *
 * Created on July 19, 2018, 6:30 AM
 */

#ifndef MODULESESSIONMESSAGEHELPER_HPP
#define MODULESESSIONMESSAGEHELPER_HPP

#include <vector>
#include <memory>
#include <string>

#include "SoftwareConsensus.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/crypto/Containers.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


namespace keto {
namespace software_consensus {


class ModuleSessionMessageHelper {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    
    ModuleSessionMessageHelper();
    ModuleSessionMessageHelper(
            const keto::proto::ModuleSessionMessage& modulesSessionMessage);
    ModuleSessionMessageHelper(const ModuleSessionMessageHelper& orig) = default;
    virtual ~ModuleSessionMessageHelper();
    
    ModuleSessionMessageHelper& setSecret(
            const keto::crypto::SecureVector& secret);
    keto::crypto::SecureVector getSecret();
    
    operator keto::proto::ModuleSessionMessage();
    keto::proto::ModuleSessionMessage getModuleSessionMessage();
    
private:
    keto::proto::ModuleSessionMessage modulesSessionMessage;
};

}
}

#endif /* MODULESESSIONMESSAGEHELPER_HPP */

