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

#include "keto/software_consensus/ConsensusHashScriptInfo.hpp"

namespace keto {
namespace software_consensus {

class ConsensusHashGenerator;
typedef std::shared_ptr<ConsensusHashGenerator> ConsensusHashGeneratorPtr;
typedef std::vector<ConsensusHashScriptInfo> ConsensusHashScriptInfoVector;
typedef keto::crypto::SecureVector (*getModuleSignature)();
typedef keto::crypto::SecureVector (*getModuleKey)();
typedef keto::crypto::SecureVector (*getSourceVersion)();
typedef std::map<std::string,getSourceVersion> SourceVersionMap;


class ConsensusHashGenerator {
public:
    ConsensusHashGenerator(const ConsensusHashScriptInfoVector& consensusVector,
            getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap);
    ConsensusHashGenerator(const ConsensusHashGenerator& orig) = delete;
    virtual ~ConsensusHashGenerator();
    
    // singleton methods
    static ConsensusHashGeneratorPtr getInstance();
    static ConsensusHashGeneratorPtr initInstance(
            const ConsensusHashScriptInfoVector consensusVector,
            getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap);
    static void finInstance();
    
    // method to set the session key
    keto::crypto::SecureVector decryptWithSessionKey(
        const std::vector<uint8_t>& bytes);
    void setSession(
        const keto::crypto::SecureVector& sessionKey);
    std::vector<uint8_t> generateSeed();
    std::vector<uint8_t> generateHash(const std::vector<uint8_t>& seed);
    
    
private:
    ConsensusHashScriptInfoVector consensusVector;
    keto::crypto::SecureVector sessionKey;
    keto::crypto::SecureVector encodedKey;
    keto::crypto::SecureVector sessionScript;
    
    // module configuration
    getModuleSignature moduleSignatureRef; 
    getModuleKey moduleKeyRef;
    SourceVersionMap sourceVersionMap;
};


}
}

#endif /* CONSENSUSHASHGENERATOR_HPP */

