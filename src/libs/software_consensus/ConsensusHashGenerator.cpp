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
#include "keto/crypto/SecureVectorUtils.hpp"

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
    keto::crypto::SecureVector seed;
    keto::asn1::HashHelper previousHash;
public:
    ChaiScriptSession(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::asn1::HashHelper& previousHash) : 
        moduleSignatureRef(moduleSignatureRef),
        moduleKeyRef(moduleKeyRef),
        sourceVersionMap(sourceVersionMap),
        sessionKey(sessionKey),
        encodedKey(encodedKey),
        previousHash(previousHash)
    {
    }
    
    ChaiScriptSession(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap,
            keto::crypto::SecureVector sessionKey,
            keto::crypto::SecureVector encodedKey,
            keto::crypto::SecureVector seed) : 
        moduleSignatureRef(moduleSignatureRef),
        moduleKeyRef(moduleKeyRef),
        sourceVersionMap(sourceVersionMap),
        sessionKey(sessionKey),
        encodedKey(encodedKey),
        seed(seed)
    {
    }
    
    getModuleSignature getModuleSignatureRef() {
        return this->moduleSignatureRef;
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
    
    keto::crypto::SecureVector getSeed() {
        return this->seed;
    }
    
    keto::crypto::SecureVector getPreviousHash() {
        return this->previousHash.operator keto::crypto::SecureVector();
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

keto::crypto::SecureVector chai_getSeed() {
    return chaiScriptSessionPtr->getSeed();
}

keto::crypto::SecureVector chai_getPreviousHash() {
    return chaiScriptSessionPtr->getPreviousHash();
}

ChaiScriptPtr initChaiScript(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::asn1::HashHelper& previousHash) {
    chaiScriptSessionPtr = std::make_shared<ChaiScriptSession>(
            moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,previousHash);
    
    ChaiScriptPtr chaiScriptPtr = std::make_shared<chaiscript::ChaiScript>();
    
    
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleSignature), "getModuleSignature");
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleKeyRef), "getModuleKeyRef");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSourceVersion), "getSourceVersion");
    chaiScriptPtr->add(chaiscript::fun(&chai_generateHash), "generateHash");
    chaiScriptPtr->add(chaiscript::fun(&chai_signBytes), "signBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_encryptBytes), "encryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_decryptBytes), "decryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_getPreviousHash), "getPreviousHash");
    
    return chaiScriptPtr;
}

ChaiScriptPtr initChaiScript(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::crypto::SecureVector& seed) {
    chaiScriptSessionPtr = std::make_shared<ChaiScriptSession>(
            moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,seed);
    
    ChaiScriptPtr chaiScriptPtr = std::make_shared<chaiscript::ChaiScript>();
    
    
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleSignature), "getModuleSignature");
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleKeyRef), "getModuleKeyRef");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSourceVersion), "getSourceVersion");
    chaiScriptPtr->add(chaiscript::fun(&chai_generateHash), "generateHash");
    chaiScriptPtr->add(chaiscript::fun(&chai_signBytes), "signBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_encryptBytes), "encryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_decryptBytes), "decryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSeed), "getSeed");
    
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
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::asn1::HashHelper& previousHash) {
        this->chaiScriptPtr = initChaiScript(moduleSignatureRef,moduleKeyRef,sourceVersionMap,
                sessionKey,encodedKey,previousHash);
    }
    
    ChaiScriptSessionScope(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::crypto::SecureVector& seed) {
        this->chaiScriptPtr = initChaiScript(moduleSignatureRef,moduleKeyRef,sourceVersionMap,
                sessionKey,encodedKey,seed);
    }
    
    ~ChaiScriptSessionScope() {
        finChaiScript();
    }
    
    ChaiScriptPtr getChaiScriptPtr() {
        return this->chaiScriptPtr;
    }
};


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
ConsensusHashGeneratorPtr ConsensusHashGenerator::initInstance(
        const ConsensusHashScriptInfoVector consensusVector,
        getModuleSignature moduleSignatureRef,
        getModuleKey moduleKeyRef,
        SourceVersionMap sourceVersionMap) {
    return ConsensusHashGeneratorPtr(new ConsensusHashGenerator(consensusVector,
            moduleSignatureRef, moduleKeyRef, sourceVersionMap));
}

// method to set the session key
void ConsensusHashGenerator::setSession(
        const keto::crypto::SecureVector& sessionKey) {
    this->sessionKey = sessionKey;
    for (ConsensusHashScriptInfoPtr consensusScript : this->consensusVector) {
        
        try {
            keto::key_tools::ContentDecryptor contentDecryptor(sessionKey,
                consensusScript->getEncodedKey(),consensusScript->getCode());

            if (keto::crypto::HashGenerator().generateHash(
                    contentDecryptor.getContent()) == consensusScript->getHash()) {
                this->sessionScript = contentDecryptor.getContent();
                this->encodedKey = consensusScript->getEncodedKey();
                break;
            }
        } catch (...) {
            // the key is invalid as a result we ignore and move on.
        }
    }
}

keto::crypto::SecureVector ConsensusHashGenerator::generateSeed(const keto::asn1::HashHelper& previousHash) {
    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,previousHash);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();
    
    std::string code(this->sessionScript.begin(),this->sessionScript.end());
    return chaiScriptPtr->eval<keto::crypto::SecureVector>(code);
    
}

keto::crypto::SecureVector ConsensusHashGenerator::generateHash(const keto::crypto::SecureVector& seed) {
    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,seed);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();
    
    std::string code(this->sessionScript.begin(),this->sessionScript.end());
    return chaiScriptPtr->eval<keto::crypto::SecureVector>(code);
}

}
}
