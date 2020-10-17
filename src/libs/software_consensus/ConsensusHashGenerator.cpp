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
#include <sstream>

#include "botan/hex.h"

#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/key_tools/ContentDecryptor.hpp"
#include "keto/key_tools/ContentEncryptor.hpp"
#include "keto/key_tools/ContentSigner.hpp"

#include "keto/crypto/HashGenerator.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"
#include "keto/software_consensus/ConsensusHashScriptInfo.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"
#include "keto/software_consensus/Exception.hpp"


namespace keto {
namespace software_consensus {
    
std::string ConsensusHashGenerator::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

    
class ChaiScriptSession {
private:
    getModuleSignature moduleSignatureRef; 
    getModuleKey moduleKeyRef;
    SourceVersionMap sourceVersionMap;
    keto::crypto::SecureVector sessionKey;
    keto::crypto::SecureVector encodedKey;
    keto::crypto::SecureVector seed;
    keto::asn1::HashHelper previousHash;
    CallBacks callBacks;
public:
    ChaiScriptSession(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::asn1::HashHelper& previousHash,
            const CallBacks& callBacks) :
        moduleSignatureRef(moduleSignatureRef),
        moduleKeyRef(moduleKeyRef),
        sourceVersionMap(sourceVersionMap),
        sessionKey(sessionKey),
        encodedKey(encodedKey),
        previousHash(previousHash),
        callBacks(callBacks)
    {
    }
    
    ChaiScriptSession(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            SourceVersionMap sourceVersionMap,
            keto::crypto::SecureVector sessionKey,
            keto::crypto::SecureVector encodedKey,
            keto::crypto::SecureVector seed,
            const CallBacks& callBacks) :
        moduleSignatureRef(moduleSignatureRef),
        moduleKeyRef(moduleKeyRef),
        sourceVersionMap(sourceVersionMap),
        sessionKey(sessionKey),
        encodedKey(encodedKey),
        seed(seed),
        callBacks(callBacks)
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

    CallBacks getCallBacks() {
        return this->callBacks;
    }
};

typedef std::shared_ptr<chaiscript::ChaiScript> ChaiScriptPtr;
typedef std::shared_ptr<ChaiScriptSession> ChaiScriptSessionPtr;

thread_local ChaiScriptSessionPtr chaiScriptSessionPtr; 


std::string chai_getModuleSignature() {
    return keto::crypto::SecureVectorUtils().copySecureToString((chaiScriptSessionPtr->getModuleSignatureRef())());
}

std::string chai_getModuleKeyRef() {
    return keto::crypto::SecureVectorUtils().copySecureToString((chaiScriptSessionPtr->getModuleKeyRef())());
}

std::string chai_getSourceVersion(const std::string& name) {
    return chaiScriptSessionPtr->getSourceVersionMap()[name]();
}

std::string chai_generateHash(const std::string& value) {
    return keto::crypto::SecureVectorUtils().copySecureToString(keto::crypto::HashGenerator().generateHash(value));
}

std::string chai_signBytes(const std::string& value) {
    return keto::crypto::SecureVectorUtils().copySecureToString(
            keto::key_tools::ContentSigner(chaiScriptSessionPtr->getSessionKey(),
            chaiScriptSessionPtr->getEncodedKey(),
            keto::crypto::SecureVectorUtils().copyStringToSecure(value)).getSignature());
}

std::string chai_encryptBytes(const std::string& value) {
    
    return keto::server_common::VectorUtils().copyVectorToString(
            keto::key_tools::ContentEncryptor(chaiScriptSessionPtr->getSessionKey(),
            chaiScriptSessionPtr->getEncodedKey(),
            keto::crypto::SecureVectorUtils().copyStringToSecure(value)).getEncryptedContent());
}

std::string chai_decryptBytes(const std::string& value) {
    return keto::crypto::SecureVectorUtils().copySecureToString(
            keto::key_tools::ContentDecryptor(chaiScriptSessionPtr->getSessionKey(),
            chaiScriptSessionPtr->getEncodedKey(),
            keto::crypto::SecureVectorUtils().copyStringToSecure(value)).getContent());
}

std::string chai_getSeed() {
    return keto::crypto::SecureVectorUtils().copySecureToString(chaiScriptSessionPtr->getSeed());
}

std::string chai_getPreviousHash() {
    return keto::crypto::SecureVectorUtils().copySecureToString(chaiScriptSessionPtr->getPreviousHash());
}

std::string chai_executeCallBacks(const std::string& value) {
    keto::crypto::SecureVector result = keto::crypto::SecureVectorUtils().copyStringToSecure(value);
    for (testCallBack callBack: chaiScriptSessionPtr->getCallBacks()) {
        result = callBack(result);
    }
    return keto::crypto::SecureVectorUtils().copySecureToString(result);
}

std::string chai_concat(const std::string& lhs,const std::string& rhs) {
    keto::crypto::SecureVector result = keto::crypto::SecureVectorUtils().copyStringToSecure(lhs);
    result.insert(result.end(),rhs.begin(),rhs.end());
    return keto::crypto::SecureVectorUtils().copySecureToString(result);
}


bool chai_vectorEqual(const std::string &lhs,const std::string &rhs) {
    return lhs == rhs;
}

ChaiScriptPtr initChaiScript(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::asn1::HashHelper& previousHash,
            const CallBacks& callBacks) {
    chaiScriptSessionPtr = std::make_shared<ChaiScriptSession>(
            moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,previousHash,callBacks);
    
    ChaiScriptPtr chaiScriptPtr = std::make_shared<chaiscript::ChaiScript>();
    
    
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleSignature), "getModuleSignature");
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleKeyRef), "getModuleKeyRef");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSourceVersion), "getSourceVersion");
    chaiScriptPtr->add(chaiscript::fun(&chai_generateHash), "generateHash");
    chaiScriptPtr->add(chaiscript::fun(&chai_signBytes), "signBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_encryptBytes), "encryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_decryptBytes), "decryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_getPreviousHash), "getPreviousHash");
    chaiScriptPtr->add(chaiscript::fun(&chai_getPreviousHash), "getSeed");
    chaiScriptPtr->add(chaiscript::fun(&chai_executeCallBacks), "executeCallBacks");
    chaiScriptPtr->add(chaiscript::fun(&chai_concat), "avertemConcat");
    chaiScriptPtr->add(chaiscript::fun(&chai_vectorEqual), "vectorEqual");
    
    return chaiScriptPtr;
}

ChaiScriptPtr initChaiScript(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::crypto::SecureVector& seed,
            const CallBacks& callBacks) {
    chaiScriptSessionPtr = std::make_shared<ChaiScriptSession>(
            moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,seed,callBacks);
    
    ChaiScriptPtr chaiScriptPtr = std::make_shared<chaiscript::ChaiScript>();
    
    
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleSignature), "getModuleSignature");
    chaiScriptPtr->add(chaiscript::fun(&chai_getModuleKeyRef), "getModuleKeyRef");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSourceVersion), "getSourceVersion");
    chaiScriptPtr->add(chaiscript::fun(&chai_generateHash), "generateHash");
    chaiScriptPtr->add(chaiscript::fun(&chai_signBytes), "signBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_encryptBytes), "encryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_decryptBytes), "decryptBytes");
    chaiScriptPtr->add(chaiscript::fun(&chai_getSeed), "getSeed");
    chaiScriptPtr->add(chaiscript::fun(&chai_executeCallBacks), "executeCallBacks");
    chaiScriptPtr->add(chaiscript::fun(&chai_concat), "avertemConcat");
    chaiScriptPtr->add(chaiscript::fun(&chai_vectorEqual), "vectorEqual");
    
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
            const keto::asn1::HashHelper& previousHash,
            const CallBacks& callBacks) {
        this->chaiScriptPtr = initChaiScript(moduleSignatureRef,moduleKeyRef,sourceVersionMap,
                sessionKey,encodedKey,previousHash,callBacks);
    }
    
    ChaiScriptSessionScope(getModuleSignature moduleSignatureRef,
            getModuleKey moduleKeyRef,
            const SourceVersionMap& sourceVersionMap,
            const keto::crypto::SecureVector& sessionKey,
            const keto::crypto::SecureVector& encodedKey,
            const keto::crypto::SecureVector& seed,
            const CallBacks& callBacks) {
        this->chaiScriptPtr = initChaiScript(moduleSignatureRef,moduleKeyRef,sourceVersionMap,
                sessionKey,encodedKey,seed,callBacks);
    }
    
    ~ChaiScriptSessionScope() {
        // check out the chai script ptr
        this->chaiScriptPtr->set_state(chaiscript::ChaiScript::State());
        this->chaiScriptPtr.reset();
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

void ConsensusHashGenerator::registerCallBacks(const CallBacks& callBacks) {
    this->callBacks = callBacks;
}

// method to set the session key
bool ConsensusHashGenerator::setSession(
        const keto::crypto::SecureVector& sessionKey) {
    std::unique_lock<std::mutex> guard(classMutex);
    if (this->sessionKey == sessionKey) {
        return false;
    } else {
        bool validSession = false;
        for (ConsensusHashScriptInfoPtr consensusScript : this->consensusVector) {

            try {
                keto::key_tools::ContentDecryptor contentDecryptor(sessionKey,
                                                                   consensusScript->getEncodedKey(),
                                                                   consensusScript->getCode());

                if (keto::crypto::HashGenerator().generateHash(
                        contentDecryptor.getContent()) == consensusScript->getHash()) {
                    this->sessionScript = consensusScript->getCode();
                    this->encodedKey = consensusScript->getEncodedKey();
                    this->sessionShortScript = consensusScript->getShortCode();
                    validSession = true;
                    break;
                }
            } catch (...) {
                // the key is invalid as a result we ignore and move on.
            }
        }
        if (!validSession) {
            std::stringstream ss;
            ss << "The session key is invalid [" << Botan::hex_encode(sessionKey, true) << "]";
            BOOST_THROW_EXCEPTION(keto::software_consensus::InvalidSessionException(ss.str()));
        }
        this->sessionKey = sessionKey;
        this->currentSoftwareHash.clear();
        return true;
    }
}

keto::crypto::SecureVector ConsensusHashGenerator::generateSeed(const keto::asn1::HashHelper& previousHash) {
    std::unique_lock<std::mutex> guard(classMutex);
    keto::key_tools::ContentDecryptor contentDecryptor(this->sessionKey,this->encodedKey,this->sessionScript);
    keto::crypto::SecureVector secureVector = contentDecryptor.getContent();

    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,previousHash,
            this->callBacks);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();
    
    std::string code(secureVector.begin(),secureVector.end());
    try {
        return keto::crypto::SecureVectorUtils().copyStringToSecure(chaiScriptPtr->eval<std::string>(code));
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[generateSeed] Failed to execute code [" << code << "] Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[generateSeed] Failed to execute code [" << code << "] Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[generateSeed] Failed to execute code [" << code << "] Cause: " << ex.what();
        throw;
    } catch (...) {
        KETO_LOG_ERROR << "[generateSeed] Failed to execute code [" << code << "]";
        throw;
    }
}


keto::crypto::SecureVector ConsensusHashGenerator::generateHash(const keto::crypto::SecureVector& seed) {
    std::unique_lock<std::mutex> guard(classMutex);
    keto::key_tools::ContentDecryptor contentDecryptor(this->sessionKey,this->encodedKey,this->sessionScript);
    keto::crypto::SecureVector secureVector = contentDecryptor.getContent();

    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,seed,
            this->callBacks);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();
    
    std::string code(secureVector.begin(),secureVector.end());
    try {
        if (currentSoftwareHash.size()) {
            return keto::crypto::SecureVectorUtils().copyStringToSecure(chaiScriptPtr->eval<std::string>(code));
        } else {
            return this->currentSoftwareHash = keto::crypto::SecureVectorUtils().copyStringToSecure(
                    chaiScriptPtr->eval<std::string>(code));
        }
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "] Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "] Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "] Cause: " << ex.what();
        throw;
    } catch (...) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "]";
        throw;
    }
}


keto::crypto::SecureVector ConsensusHashGenerator::getCurrentSoftwareHash() {
    return this->currentSoftwareHash;
}

keto::crypto::SecureVector ConsensusHashGenerator::generateSessionHash(const keto::crypto::SecureVector& name) {
    std::unique_lock<std::mutex> guard(classMutex);
    keto::key_tools::ContentDecryptor contentDecryptor(this->sessionKey,this->encodedKey,this->sessionShortScript);
    keto::crypto::SecureVector secureVector = contentDecryptor.getContent();

    ChaiScriptSessionScope scope(moduleSignatureRef,moduleKeyRef,sourceVersionMap,sessionKey,encodedKey,name,
            this->callBacks);
    ChaiScriptPtr chaiScriptPtr = scope.getChaiScriptPtr();

    std::string code(secureVector.begin(),secureVector.end());
    try {
        return keto::crypto::SecureVectorUtils().copyStringToSecure(chaiScriptPtr->eval<std::string>(code));
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "] Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "] Cause: " << boost::diagnostic_information(ex,true);
        throw;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "] Cause: " << ex.what();
        throw;
    } catch (...) {
        KETO_LOG_ERROR << "[generateHash] Failed to execute code [" << code << "]";
        throw;
    }
}

std::vector<uint8_t> ConsensusHashGenerator::encrypt(const keto::crypto::SecureVector& value) {
    keto::key_tools::ContentEncryptor contentEncryptor(this->sessionKey,this->encodedKey,value);
    return contentEncryptor.getEncryptedContent();
}

keto::crypto::SecureVector ConsensusHashGenerator::decrypt(const std::vector<uint8_t>& value) {
    keto::key_tools::ContentDecryptor contentDecryptor(this->sessionKey,this->encodedKey,value);
    return contentDecryptor.getContent();
}


}
}
