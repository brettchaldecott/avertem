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

#ifndef MODULEHASHMESSAGEHELPER_HPP
#define MODULEHASHMESSAGEHELPER_HPP

#include <vector>
#include <memory>
#include <string>

#include "SoftwareConsensus.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"

#include "keto/crypto/Containers.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


namespace keto {
namespace software_consensus {


class ModuleHashMessageHelper {
public:
    inline static std::string getVersion() {
        return OBFUSCATED("$Id: cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $");
    };
    
    static std::string getSourceVersion();

    
    ModuleHashMessageHelper();
    ModuleHashMessageHelper(
            const keto::proto::ModuleHashMessage& modulesHashMessage);
    ModuleHashMessageHelper(const ModuleHashMessageHelper& orig) = default;
    virtual ~ModuleHashMessageHelper();
    
    ModuleHashMessageHelper& setHash(
            const keto::crypto::SecureVector& secret);
    keto::asn1::HashHelper getHash();
    keto::crypto::SecureVector getHash_lock();
    
    operator keto::proto::ModuleHashMessage();
    keto::proto::ModuleHashMessage getModuleHashMessage();
    
private:
    keto::proto::ModuleHashMessage modulesHashMessage;
};

}
}

#endif /* MODULESESSIONMESSAGEHELPER_HPP */

