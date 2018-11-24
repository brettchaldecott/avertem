/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusHashScriptInfo.hpp
 * Author: ubuntu
 *
 * Created on June 29, 2018, 11:07 AM
 */

#ifndef CONSENSUSHASHSCRIPTINFO_HPP
#define CONSENSUSHASHSCRIPTINFO_HPP

#include <vector>
#include <string>
#include <memory>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/server_common/VectorUtils.hpp"
#include "keto/crypto/Containers.hpp"


namespace keto {
namespace software_consensus {

class ConsensusHashScriptInfo;
typedef std::shared_ptr<ConsensusHashScriptInfo> ConsensusHashScriptInfoPtr;
typedef keto::crypto::SecureVector (*getHash)();
typedef keto::crypto::SecureVector (*getEncodedKey)();
typedef std::vector<uint8_t> (*getCode)();


class ConsensusHashScriptInfo {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    
    ConsensusHashScriptInfo(keto::software_consensus::getHash hashFunctionRef, 
            keto::software_consensus::getEncodedKey keyFunctionRef,
            keto::software_consensus::getCode codeFunctionRef);
    ConsensusHashScriptInfo(const ConsensusHashScriptInfo& orig) = default;
    virtual ~ConsensusHashScriptInfo();
    
    keto::crypto::SecureVector getHash();
    keto::crypto::SecureVector getEncodedKey();
    std::vector<uint8_t> getCode();
private:
    keto::software_consensus::getHash hashFunctionRef;
    keto::software_consensus::getEncodedKey keyFunctionRef;
    keto::software_consensus::getCode codeFunctionRef;
    
};


}
}

#endif /* CONSENSUSHASHSCRIPTINFO_HPP */

