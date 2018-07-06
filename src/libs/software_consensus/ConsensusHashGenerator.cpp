/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusHashGenerator.cpp
 * Author: ubuntu
 * 
 * Created on June 29, 2018, 11:01 AM
 */

#include <condition_variable>

#include <chaiscript/chaiscript.hpp>
#include <map>

#include "keto/crypto/HashGenerator.hpp"

#include "keto/key_tools/ContentDecryptor.hpp"
#include "keto/key_tools/ContentEncryptor.hpp"
#include "keto/key_tools/ContentSigner.hpp"

#include "keto/crypto/HashGenerator.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"
#include "keto/software_consensus/ConsensusHashScriptInfo.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"


namespace keto {
namespace software_consensus {
    
class ChaiScriptSession {
private:
    getModuleSignature moduleSignatureRef; 
    getModuleKey moduleKeyRef;
    SourceVersionMap sourceVersionMap;
    keto::crypto::SecureVector sessionKey;
    keto::crypto::SecureVector encodedKey;
    
public:
    ChaiScriptSession(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap,
            keto::crypto::SecureVector sessionKey,
            keto::crypto::SecureVector encodedKey) : 
        moduleSignatureRef(moduleSignatureRef),
        moduleKeyRef(moduleKeyRef),
        sourceVersionMap(sourceVersionMap),
        sessionKey(sessionKey),
        encodedKey(encodedKey)
    {
    }
    
    
    getModuleSignature getModuleSignatureRef() {
        return this->moduleKeyRef;
    }
    
    getModuleKey getModuleKeyRef() {
        return this->moduleKeyRef;
    }
    
    SourceVersionMap getSourceVersionMap() {
        return this->sourceVersionMap;
    }
    
    keto::crypto::SecureVector getSessionKey() {
        return this->sessionKey;
    }
    
    keto::crypto::SecureVector getEncodedKey() {
        return this->encodedKey;
    }
    
};

typedef std::shared_ptr<chaiscript::ChaiScript> ChaiScriptPtr;
typedef std::shared_ptr<ChaiScriptSession> ChaiScriptSessionPtr;

thread_local ChaiScriptSessionPtr chaiScriptSessionPtr; 


keto::crypto::SecureVector chai_getModuleSignature() {
    return (chaiScriptSessionPtr->getModuleSignatureRef())();
}

keto::crypto::SecureVector chai_getModuleKeyRef() {
    return (chaiScriptSessionPtr->getModuleKeyRef())();
}

keto::crypto::SecureVector chai_getSourceVersion(const std::string &name) {
    return chaiScriptSessionPtr->getSourceVersionMap()[name]();
}

keto::crypto::SecureVector chai_generateHash(const keto::crypto::SecureVector &value) {
    return keto::crypto::HashGenerator().generateHash(value);
}

keto::crypto::SecureVector chai_signBytes(const keto::crypto::SecureVector &value) {
    return keto::key_tools::ContentSigner(chaiScriptSessionPtr->getSessionKey(),
            chaiScriptSessionPtr->getEncodedKey(),value).getSignature();
}

keto::crypto::SecureVector chai_encryptBytes(const keto::crypto::SecureVector &value) {
    
    return keto::key_tools::ContentEncryptor(chaiScriptSessionPtr->getSessionKey(),
            chaiScriptSessionPtr->getEncodedKey(),value).getEncryptedContent_locked();
}

keto::crypto::SecureVector chai_decryptBytes(const keto::crypto::SecureVector &value) {
    return keto::key_tools::ContentDecryptor(chaiScriptSessionPtr->getSessionKey(),
            chaiScriptSessionPtr->getEncodedKey(),value).getContent();
}


ChaiScriptPtr initChaiScript(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap,
            keto::crypto::SecureVector sessionKey,
            keto::crypto::SecureVector encodedKey) {
    chaiScriptSessionPtr = std::make_shared<ChaiScriptSession>(
            moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey);
    
    ChaiScriptPtr chaiScriptPtr = std::make_shared<chaiscript::ChaiScript>();
    
    
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleSignature), "getModuleSignature");
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleKeyRef), "getModuleKeyRef");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSourceVersion), "getSourceVersion");
    chaiScriptPtr->add(chaiscript::fun(&chai_generateHash), "generateHash");
    chaiScriptPtr->add(chaiscript::fun(&chai_signBytes), "signBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_encryptBytes), "encryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_decryptBytes), "decryptBytes");
    
    return chaiScriptPtr;
}



void finChaiScript() {
    chaiScriptSessionPtr.reset();
}

class ChaiScriptSessionScope {
private:
    ChaiScriptPtr chaiScriptPtr;
public:
    ChaiScriptSessionScope(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap,
            keto::crypto::SecureVector sessionKey,
            keto::crypto::SecureVector encodedKey) {
        this->chaiScriptPtr = initChaiScript(moduleSignatureRef,moduleKeyRef,sourceVersionMap,
                sessionKey,encodedKey);
    }
    
    ~ChaiScriptSessionScope() {
        finChaiScript();
    }
    
    ChaiScriptPtr getChaiScriptPtr() {
        return this->chaiScriptPtr;
    }
};


// singleton
static ConsensusHashGeneratorPtr singleton;
    
ConsensusHashGenerator::ConsensusHashGenerator(
        const ConsensusHashScriptInfoVector& consensusVector,
        getModuleSignature moduleSignatureRef,
        getModuleKey moduleKeyRef,
        SourceVersionMap sourceVersionMap) : 
        consensusVector(consensusVector), moduleSignatureRef(moduleSignatureRef),
        moduleKeyRef(moduleKeyRef), sourceVersionMap(sourceVersionMap) {
    
    
}

ConsensusHashGenerator::~ConsensusHashGenerator() {
}

// singleton methods
ConsensusHashGeneratorPtr ConsensusHashGenerator::getInstance() {
    return singleton;
}

ConsensusHashGeneratorPtr ConsensusHashGenerator::initInstance(
        const ConsensusHashScriptInfoVector consensusVector,
        getModuleSignature moduleSignatureRef,
        getModuleKey moduleKeyRef,
        SourceVersionMap sourceVersionMap) {
    return singleton =  std::make_shared<ConsensusHashGenerator>(consensusVector,
            moduleSignatureRef, moduleKeyRef, sourceVersionMap);
}

void ConsensusHashGenerator::finInstance() {
    if (singleton) {
        singleton.reset();
    }
}

// method to set the session key
void ConsensusHashGenerator::setSession(
        const keto::crypto::SecureVector& sessionKey) {
    this->sessionKey = sessionKey;
    for (ConsensusHashScriptInfo consensusScript : this->consensusVector) {
        
        keto::key_tools::ContentDecryptor contentDecryptor(sessionKey,
            consensusScript.getEncodedKey(),consensusScript.getCode());
        
        if (keto::crypto::HashGenerator().generateHash(
                contentDecryptor.getContent()) == consensusScript.getHash()) {
            this->sessionScript = contentDecryptor.getContent();
            this->encodedKey = consensusScript.getEncodedKey();
        }
    }
}

keto::crypto::SecureVector ConsensusHashGenerator::decryptWithSessionKey(
        const std::vector<uint8_t>& bytes) {
    return keto::key_tools::ContentDecryptor(this->sessionKey,this->encodedKey,
            bytes).getContent();
}

std::vector<uint8_t> ConsensusHashGenerator::generateSeed() {
    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();
    
}

std::vector<uint8_t> ConsensusHashGenerator::generateHash(const std::vector<uint8_t>& seed) {
    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();
    
}

}
}
