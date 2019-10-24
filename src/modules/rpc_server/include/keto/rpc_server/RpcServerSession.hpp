/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RpcServerSession.hpp
 * Author: ubuntu
 *
 * Created on August 18, 2018, 1:05 PM
 */

#ifndef RPCSERVERSESSION_HPP
#define RPCSERVERSESSION_HPP

#include <memory>
#include <map>
#include <vector>
#include <mutex>

namespace keto {
namespace rpc_server {

class RpcServerSession;
typedef std::shared_ptr<RpcServerSession> RpcServerSessionPtr;
    
class RpcServerSession {
public:
    RpcServerSession();
    RpcServerSession(const RpcServerSession& orig) = default;
    virtual ~RpcServerSession();
    
    static RpcServerSessionPtr init();
    static RpcServerSessionPtr getInstance();
    static void fin();

    std::vector<std::string> handlePeers(const std::vector<uint8_t>& account,
            const std::string& host);
    void addPeer(const std::vector<uint8_t>& account,
            const std::string& host);
    std::vector<std::string> getPeers(
            const std::vector<uint8_t> account);
    std::vector<std::string> getPeers(
            const std::vector<std::vector<uint8_t>>& accounts);
    
private:
    std::recursive_mutex classMutex;
    std::map<std::vector<uint8_t>,std::string> accountPeerCache;
    std::vector<std::string> accountPeerList;
    
};

}
}

#endif /* RPCSERVERSESSION_HPP */

