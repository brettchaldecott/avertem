/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RouterService.hpp
 * Author: ubuntu
 *
 * Created on March 2, 2018, 4:04 PM
 */

#ifndef ROUTERSERVICE_HPP
#define ROUTERSERVICE_HPP

#include <string>
#include <memory>

#include "Protocol.pb.h"

#include "keto/common/MetaInfo.hpp"

#include "keto/event/Event.hpp"

#include "keto/crypto/KeyLoader.hpp"

#include "keto/router_utils/RpcPeerHelper.hpp"

#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"


namespace keto {
namespace router {
        

class RouterService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    RouterService(const RouterService& orig) = delete;
    virtual ~RouterService();
    
    static std::shared_ptr<RouterService> init();
    static void fin();
    static std::shared_ptr<RouterService> getInstance();
    
    
    keto::event::Event routeMessage(const keto::event::Event& event);
    keto::event::Event registerRpcPeerClient(const keto::event::Event& event);
    keto::event::Event registerRpcPeerServer(const keto::event::Event& event);
    keto::event::Event processPushRpcPeer(const keto::event::Event& event);
    keto::event::Event deregisterRpcPeer(const keto::event::Event& event);
    keto::event::Event activateRpcPeer(const keto::event::Event& event);
    keto::event::Event registerService(const keto::event::Event& event);
    keto::event::Event updateStateRouteMessage(const keto::event::Event& event);
    
    
private:
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;

    RouterService();
    
    /**
     * This method is called to route a message locally.
     * 
     * @param messageWrapper The message wrapper
     */
    void routeLocal(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper);
    
    /**
     * This method is called to route a message locally.
     * 
     * @param messageWrapper The message wrapper
     */
    void routeToAccount(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper);
    
    
    void routeToRpcClient(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper,
            keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    
    void routeToRpcPeer(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper);
};


}
}

#endif /* ROUTERSERVICE_HPP */

