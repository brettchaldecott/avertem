/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusSessionManager.hpp
 * Author: ubuntu
 *
 * Created on July 18, 2018, 5:25 PM
 */

#ifndef CONSENSUSSESSIONMANAGER_HPP
#define CONSENSUSSESSIONMANAGER_HPP

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ConsensusSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    ConsensusSessionManager();
    ConsensusSessionManager(const ConsensusSessionManager& orig) = default;
    virtual ~ConsensusSessionManager();
    
    void updateSessionKey(const keto::crypto::SecureVector& sessionKey);
private:

};


}
}


#endif /* CONSENSUSSESSIONMANAGER_HPP */

