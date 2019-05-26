/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusServer.hpp
 * Author: ubuntu
 * 
 * Created on August 12, 2018, 8:38 AM
 */

#ifndef CONSENSUSSERVER_HPP
#define CONSENSUSSERVER_HPP

#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <chrono>


#include <iostream>
#include <boost/asio.hpp>

#include <keto/crypto/Containers.hpp>

namespace keto {
namespace consensus_module {

class ConsensusServer;
typedef std::shared_ptr<ConsensusServer> ConsensusServerPtr;
    
class ConsensusServer {
public:
    ConsensusServer();
    ConsensusServer(const ConsensusServer& orig) = delete;
    virtual ~ConsensusServer();
    
    bool require();
    void start();
    
    void process();
private:
    static const int THREAD_COUNT;
    std::shared_ptr<boost::asio::io_context> ioc;
    std::shared_ptr<boost::asio::deadline_timer> timer;
    std::vector<std::thread> threadsVector;
    std::vector<std::string> sessionKeys;
    int currentPos;
    std::chrono::system_clock::time_point sessionkeyPoint;
    std::chrono::system_clock::time_point networkPoint;
    std::chrono::system_clock::time_point networkHeartbeatPoint;
    int netwokSessionLength;
    int netwokProtocolDelay;
    int networkHeartbeatDelay;
    int networkHeartbeatSlot;

    void internalConsensusInit(const keto::crypto::SecureVector& initHash);
    void internalConsensusProtocolCheck(const keto::crypto::SecureVector& initHash);
    void initNetworkHeartbeat();
    void reschedule();

};


}
}

#endif /* CONSENSUSSERVER_HPP */

