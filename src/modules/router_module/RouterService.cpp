/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RouterService.cpp
 * Author: ubuntu
 * 
 * Created on March 2, 2018, 4:04 PM
 */

#include <iostream>
#include <vector>

#include <botan/hex.h>

#include "BlockChain.pb.h"

#include "Protocol.pb.h"
#include "Account.pb.h"
#include "Route.pb.h"

#include "keto/common/Log.hpp"
#include "keto/router/RouterService.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/router_db/RouterStore.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/router/RouterService.hpp"
#include "keto/router/RouterRegistry.hpp"
#include "keto/router/TangleServiceCache.hpp"

#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/router_utils/AccountRoutingStoreHelper.hpp"
#include "keto/router/PeerCache.hpp"


namespace keto {
namespace router {

static std::shared_ptr<RouterService> singleton;

std::string RouterService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RouterService::RouterService() {
}

RouterService::~RouterService() {
}

std::shared_ptr<RouterService> RouterService::init() {
    if (!singleton) {
        singleton = std::shared_ptr<RouterService>(new RouterService());
    }
    return singleton;
}

void RouterService::fin() {
    singleton.reset();
}

std::shared_ptr<RouterService> RouterService::getInstance() {
    return singleton;
}


keto::event::Event RouterService::routeMessage(const keto::event::Event& event) {
    keto::proto::MessageWrapper  messageWrapper = 
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    
    keto::asn1::HashHelper accountHash(messageWrapper.account_hash());
    std::cout << "Attempt to route the account hash : " 
            << accountHash.getHash(keto::common::StringEncoding::HEX) << std::endl;
    // look to see if the message account is for this server
    if (keto::crypto::SecureVectorUtils().copyFromSecure(
            accountHash.operator keto::crypto::SecureVector()) == keto::server_common::ServerInfo::getInstance()->getAccountHash()) {
        
        routeLocal(messageWrapper);
        // the result of the local routing 
        keto::proto::MessageWrapperResponse response;
        response.set_success(true);
        response.set_result("local");
        return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
    }

    if (!TangleServiceCache::getInstance()->containsAccount(messageWrapper.account_hash())) {
        keto::proto::AccountChainTangle accountChainTangle;
        accountChainTangle.set_account_id(messageWrapper.account_hash());
        accountChainTangle =
                keto::server_common::fromEvent<keto::proto::AccountChainTangle>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::AccountChainTangle>(
                                keto::server_common::Events::GET_ACCOUNT_TANGLE,accountChainTangle)));
        if (!accountChainTangle.found()) {
            messageWrapper.set_account_hash(
                    TangleServiceCache::getInstance()->getGrowTangle()->
                            getService(keto::server_common::Constants::SERVICE::BALANCE)->getAccountHash());
        } else {
            messageWrapper.set_account_hash(
                    TangleServiceCache::getInstance()->getTangle(accountChainTangle.chain_tangle_id())->
                    getService(keto::server_common::Constants::SERVICE::BALANCE)->getAccountHash());
        }
    }

    // check if we can rout to a peer
    keto::proto::RpcPeer rpcPeer;
    keto::asn1::HashHelper peerAccountHash(messageWrapper.account_hash());
    while (keto::router_db::RouterStore::getInstance()->getAccountRouting(
            peerAccountHash,rpcPeer)) {
        keto::router_utils::RpcPeerHelper rpcPeerHelper(
                rpcPeer);
        if (PeerCache::getInstance()->contains(rpcPeerHelper.getAccountHash())) {
            this->routeToRpcClient(messageWrapper,
                    PeerCache::getInstance()->getPeer(rpcPeerHelper.getAccountHash()));

            // route
            keto::proto::MessageWrapperResponse response;
            response.set_success(true);
            response.set_result("routed");
            return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
        } else {
            peerAccountHash = rpcPeerHelper.getPeerAccountHash();
        }
    }
    
    this->routeToRpcPeer(messageWrapper);
    
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    response.set_result("to peer");
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}

keto::event::Event RouterService::registerRpcPeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper  rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    rpcPeerHelper.setPeerAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    PeerCache::getInstance()->addPeer(rpcPeerHelper);

    keto::router_db::RouterStore::getInstance()->pushPeerRouting(rpcPeerHelper);


    
    return event;
}

keto::event::Event RouterService::updateStateRouteMessage(const keto::event::Event& event) {
    
    keto::proto::MessageWrapper  messageWrapper = 
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(messageWrapper);
    
    
    
    keto::transaction_common::TransactionProtoHelperPtr transactionProtoHelper =
            messageWrapperProtoHelper.getTransaction();
            
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelper =
            transactionProtoHelper->getTransactionMessageHelper();
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr = 
            transactionMessageHelper->getTransactionWrapper();
    if (transactionWrapperHelperPtr->incrementStatus() == Status_processing) {
        transactionWrapperHelperPtr->incrementStatus();
    }
    
    transactionProtoHelper->setTransaction(transactionMessageHelper);
    keto::proto::Transaction transaction = transactionProtoHelper->operator keto::proto::Transaction&();
    messageWrapperProtoHelper.setTransaction(transactionProtoHelper);
    
    messageWrapper = messageWrapperProtoHelper.operator keto::proto::MessageWrapper();
    messageWrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_INIT);
    
    return routeMessage(
            keto::server_common::toEvent<keto::proto::MessageWrapper>(
            messageWrapper));
}
    

keto::event::Event RouterService::registerService(const keto::event::Event& event) {
    keto::proto::PushService  pushService = 
            keto::server_common::fromEvent<keto::proto::PushService>(event);
    
    RouterRegistry::getInstance()->registerService(
            keto::server_common::VectorUtils().copyStringToVector(pushService.account()),
            pushService.service_name());
    
    return keto::server_common::toEvent<keto::proto::PushService>(pushService);
}


void RouterService::routeLocal(keto::proto::MessageWrapper&  messageWrapper) {
    keto::proto::Transaction transaction;
    messageWrapper.msg().UnpackTo(&transaction);
    if (transaction.status() == keto::proto::TransactionStatus::DEBIT || 
            transaction.status() == keto::proto::TransactionStatus::INIT) {
        if (messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_INIT ||
                messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BALANCE);
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapper.set_account_hash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapper));
            }
        } else if (messageWrapper.message_operation() == 
                keto::proto::MessageOperation::MESSAGE_BALANCE) {
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapper));
        } else if (messageWrapper.message_operation() == 
                keto::proto::MessageOperation::MESSAGE_BLOCK) {
            
        } else if (messageWrapper.message_operation() == 
                keto::proto::MessageOperation::MESSAGE_PROCESS) {
            
        }
        
    } else if (transaction.status() == keto::proto::TransactionStatus::CREDIT) {
        if (messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_INIT ||
                messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BALANCE);
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapper.set_account_hash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapper));
            }
        } else if (messageWrapper.message_operation() == 
                keto::proto::MessageOperation::MESSAGE_BALANCE) {
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapper));
        } else if (messageWrapper.message_operation() == 
                keto::proto::MessageOperation::MESSAGE_BLOCK) {
            
        } else if (messageWrapper.message_operation() == 
                keto::proto::MessageOperation::MESSAGE_PROCESS) {
            
        }
        
    }
}

void RouterService::routeToAccount(keto::proto::MessageWrapper&  messageWrapper) {
    keto::proto::Transaction transaction;
    messageWrapper.msg().UnpackTo(&transaction);
    if (transaction.status() == keto::proto::TransactionStatus::DEBIT || 
            transaction.status() == keto::proto::TransactionStatus::INIT) {
        if (messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_INIT ||
                messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BALANCE);
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapper.set_account_hash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapper));
            }
        }
        
    } else if (transaction.status() == keto::proto::TransactionStatus::CREDIT) {
        if (messageWrapper.message_operation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BALANCE);
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapper.set_account_hash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapper));
            }
        }
    }
}


void RouterService::routeToRpcClient(keto::proto::MessageWrapper&  messageWrapper,
        keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    
    if (!rpcPeerHelper.isServer()) {
        messageWrapper.set_account_hash(rpcPeerHelper.getAccountHashString());

        try {
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                            keto::server_common::Events::RPC_SERVER_TRANSACTION,messageWrapper));
        } catch (...) {
            // must place in a queue here
            KETO_LOG_INFO << "[RouterService] " << 
                    Botan::hex_encode((uint8_t*)rpcPeerHelper.getAccountHashString().data(),
                    rpcPeerHelper.getAccountHashString().size(),true)
                    << " Failed to dispatch to the server must add to a queue here";
        }
    } else {
        // route to a peer
        routeToRpcPeer(messageWrapper);
    }
}

void RouterService::routeToRpcPeer(keto::proto::MessageWrapper&  messageWrapper) {
    try {
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_CLIENT_TRANSACTION,messageWrapper));
    } catch (...) {
        // must place in a queue here
        KETO_LOG_INFO << "[RouterService] Failed to dispatch to an approriate peer";
    }
}

}
}