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


#include "keto/software_consensus/ProtocolHeartbeatMessageHelper.hpp"

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
    void initNetworkHeartbeat(int networkSlot);
    void initNetworkHeartbeat(const keto::proto::ProtocolHeartbeatMessage& msg);

private:
    std::mutex classMutex;

    keto::crypto::SecureVector sessionHash;
    bool activeSession;
    bool accepted;
    int netwokProtocolDelay;
    int networkProtocolCount;
    ProtocolHeartbeatMessageHelper protocolHeartbeatMessageHelper;


    // protocol consenus variables
    int protolCount;
    std::chrono::system_clock::time_point protocolPoint;

    ConsensusSessionManager();

    bool checkHeartbeatTimestamp(const keto::proto::ProtocolHeartbeatMessage& msg);

};


}
}


#endif /* CONSENSUSSESSIONMANAGER_HPP */

