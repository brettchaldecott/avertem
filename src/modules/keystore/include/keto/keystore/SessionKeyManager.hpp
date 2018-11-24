/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SessionKeyManager.hpp
 * Author: ubuntu
 *
 * Created on February 17, 2018, 8:46 AM
 */

#ifndef SESSIONKEYMANAGER_HPP
#define SESSIONKEYMANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <mutex>

#include <botan/rng.h>
#include <botan/p11_randomgenerator.h>
#include <botan/auto_rng.h>


#include "keto/event/Event.hpp"
#include "keto/crypto/Containers.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace keystore {
    
class SessionKeyManager;
typedef std::shared_ptr<SessionKeyManager> SessionKeyManagerPtr;

class SessionKeyManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    SessionKeyManager();
    SessionKeyManager(const SessionKeyManager& orig) = delete;
    virtual ~SessionKeyManager();
    
    keto::event::Event requestKey(const keto::event::Event& event);
    keto::event::Event removeKey(const keto::event::Event& event);
    
    bool isSessionKeyValid(const std::vector<uint8_t>& sessionkey);
    keto::crypto::SecureVector getPrivateKey(const std::vector<uint8_t>& sessionkey);
    
private:
    std::unique_ptr<Botan::RandomNumberGenerator> rng;
    std::mutex mutex;
    std::map<std::vector<uint8_t>,keto::crypto::SecureVector> sessionKeys;
    std::map<std::vector<uint8_t>,std::vector<uint8_t>> sessionPublicKeys;

};

}
}

#endif /* SESSIONKEYMANAGER_HPP */

