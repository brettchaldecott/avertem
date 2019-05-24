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

#include <vector>
#include <memory>
#include <mutex>

#include "HandShake.pb.h"


#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ConsensusSessionManager;
typedef std::shared_ptr<ConsensusSessionManager> ConsensusSessionManagerPtr;

class ConsensusSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    ConsensusSessionManager(const ConsensusSessionManager& orig) = delete;
    virtual ~ConsensusSessionManager();

    static ConsensusSessionManagerPtr init();
    static ConsensusSessionManagerPtr getInstance();
    static void fin();

    void updateSessionKey(const keto::crypto::SecureVector& sessionKey);
    void setSession(keto::proto::ConsensusMessage& event);
    void notifyAccepted();
    bool resetProtocolCheck();
    void notifyProtocolCheck(bool master = false);

private:
    std::mutex classMutex;

    keto::crypto::SecureVector sessionHash;
    bool activeSession;
    bool accepted;
    int netwokProtocolDelay;
    int networkProtocolCount;


    // protocol consenus variables
    int protolCount;
    std::chrono::system_clock::time_point protocolPoint;

    ConsensusSessionManager();


};


}
}


#endif /* CONSENSUSSESSIONMANAGER_HPP */

