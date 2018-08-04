/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusHashGenerator.hpp
 * Author: ubuntu
 *
 * Created on June 29, 2018, 11:01 AM
 */

#ifndef CONSENSUSHASHGENERATOR_HPP
#define CONSENSUSHASHGENERATOR_HPP

#include <memory>
#include <string>
#include <vector>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/software_consensus/ConsensusHashScriptInfo.hpp"

namespace keto {
namespace software_consensus {

class ConsensusHashGenerator;
typedef std::shared_ptr<ConsensusHashGenerator> ConsensusHashGeneratorPtr;
typedef std::vector<ConsensusHashScriptInfoPtr> ConsensusHashScriptInfoVector;
typedef keto::crypto::SecureVector (*getModuleSignature)();
typedef keto::crypto::SecureVector (*getModuleKey)();
typedef std::string (*getSourceVersion)();
typedef std::map<std::string,getSourceVersion> SourceVersionMap;


class ConsensusHashGenerator {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id: cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $");
    };
    
    static std::string getSourceVersion();
    
    
    ConsensusHashGenerator(const ConsensusHashGenerator& orig) = delete;
    virtual ~ConsensusHashGenerator();
    
    // singleton methods
    static ConsensusHashGeneratorPtr initInstance(
            const ConsensusHashScriptInfoVector consensusVector,
            getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap);
    
    // method to set the session key
    void setSession(
        const keto::crypto::SecureVector& sessionKey);
    keto::crypto::SecureVector generateSeed(const keto::asn1::HashHelper& previousHash);
    keto::crypto::SecureVector generateHash(const keto::crypto::SecureVector& seed);
    
    
private:
    ConsensusHashScriptInfoVector consensusVector;
    keto::crypto::SecureVector sessionKey;
    keto::crypto::SecureVector encodedKey;
    keto::crypto::SecureVector sessionScript;
    
    // module configuration
    getModuleSignature moduleSignatureRef; 
    getModuleKey moduleKeyRef;
    SourceVersionMap sourceVersionMap;
    
    ConsensusHashGenerator(const ConsensusHashScriptInfoVector& consensusVector,
            getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap);
    
};


}
}

#endif /* CONSENSUSHASHGENERATOR_HPP */

