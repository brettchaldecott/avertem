/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.hpp
 * Author: ubuntu
 *
 * Created on March 8, 2018, 3:15 AM
 */

#ifndef EVENTREGISTRY_HPP
#define EVENTREGISTRY_HPP

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace block {

class EventRegistry {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    EventRegistry(const EventRegistry& orig) = delete;
    virtual ~EventRegistry();
    
    static void registerEventHandlers();
    static void deregisterEventHandlers();

    static keto::event::Event persistBlockMessage(const keto::event::Event& event);
    static keto::event::Event persistServerBlockMessage(const keto::event::Event& event);
    static keto::event::Event blockMessage(const keto::event::Event& event);
    
    static keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    static keto::event::Event setModuleSession(const keto::event::Event& event);
    static keto::event::Event setupNodeConsensusSession(const keto::event::Event& event);
    static keto::event::Event consensusSessionAccepted(const keto::event::Event& event);
    static keto::event::Event consensusProtocolCheck(const keto::event::Event& event);
    static keto::event::Event consensusHeartbeat(const keto::event::Event& event);
    static keto::event::Event enableBlockProducer(const keto::event::Event& event);

    static keto::event::Event getNetworkFeeInfo(const keto::event::Event& event);
    static keto::event::Event setNetworkFeeInfo(const keto::event::Event& event);
    static keto::event::Event requestBlockSync(const keto::event::Event& event);
    static keto::event::Event processBlockSyncResponse(const keto::event::Event& event);
    static keto::event::Event processRequestBlockSyncRetry(const keto::event::Event& event);

    static keto::event::Event getAccountBlockTangle(const keto::event::Event& event);

    static keto::event::Event electRpcRequest(const keto::event::Event& event);
    static keto::event::Event electRpcResponse(const keto::event::Event& event);
    static keto::event::Event electRpcProcessPublish(const keto::event::Event& event);
    static keto::event::Event electRpcProcessConfirmation(const keto::event::Event& event);

    static keto::event::Event getBlocks(const keto::event::Event& event);
    static keto::event::Event getBlockTransactions(const keto::event::Event& event);
    static keto::event::Event getTransaction(const keto::event::Event& event);
    static keto::event::Event getAccountTransactions(const keto::event::Event& event);

    static keto::event::Event isBlockSyncComplete(const keto::event::Event& event);

private:
    EventRegistry();
    
};

}
}

#endif /* EVENTREGISTRY_HPP */

