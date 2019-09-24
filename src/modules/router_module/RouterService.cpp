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

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

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
#include "keto/transaction_common/TransactionTraceBuilder.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/router_utils/AccountRoutingStoreHelper.hpp"

#include "keto/router/PeerCache.hpp"
#include "keto/router/Constants.hpp"
#include "keto/router/Exception.hpp"

namespace keto {
namespace router {

static std::shared_ptr<RouterService> singleton;

std::string RouterService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RouterService::RouterService() {

    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::router::PrivateKeyNotConfiguredException());
    }

    std::string privateKeyPath =
            config->getVariablesMap()[Constants::PRIVATE_KEY].as<std::string>();
    if (!config->getVariablesMap().count(Constants::PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::router::PublicKeyNotConfiguredException());
    }
    std::string publicKeyPath =
            config->getVariablesMap()[Constants::PUBLIC_KEY].as<std::string>();
    keyLoaderPtr = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
                                                             publicKeyPath);

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
    keto::transaction_common::MessageWrapperProtoHelper  messageWrapperProtoHelper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);

    // add to the transaction trace
    keto::asn1::HashHelper currentAccountHash = messageWrapperProtoHelper.getAccountHash();
    keto::transaction_common::TransactionProtoHelperPtr transactionProtoHelperPtr = messageWrapperProtoHelper.getTransaction();
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr = transactionProtoHelperPtr->getTransactionMessageHelper();
    transactionMessageHelperPtr->getTransactionWrapper()->addTransactionTrace(
            *keto::transaction_common::TransactionTraceBuilder::createTransactionTrace(
                    keto::server_common::ServerInfo::getInstance()->getAccountHash(),
                    this->keyLoaderPtr));
    transactionProtoHelperPtr->setTransaction(transactionMessageHelperPtr);
    messageWrapperProtoHelper.setTransaction(
            transactionProtoHelperPtr);
    messageWrapperProtoHelper.setAccountHash(currentAccountHash);

    // update the rooting
    if (!TangleServiceCache::getInstance()->containsAccount(messageWrapperProtoHelper.getAccountHash())) {
        keto::proto::AccountChainTangle requestAccountChainTangle;
        requestAccountChainTangle.set_account_id(messageWrapperProtoHelper.getSourceAccountHash());
        keto::proto::AccountChainTangle accountChainTangle =
                keto::server_common::fromEvent<keto::proto::AccountChainTangle>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::AccountChainTangle>(
                                keto::server_common::Events::GET_ACCOUNT_TANGLE,requestAccountChainTangle)));
        if (!accountChainTangle.found()) {
            messageWrapperProtoHelper.setAccountHash(
                    TangleServiceCache::getInstance()->getGrowing()->getAccountHash());
            KETO_LOG_INFO << "Attempt to route to a new growing tangle ["
                          << messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "] for source ["
                          << messageWrapperProtoHelper.getSourceAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";

        } else {
            messageWrapperProtoHelper.setAccountHash(
                    TangleServiceCache::getInstance()->getTangle(accountChainTangle.chain_tangle_id())->getAccountHash());
            KETO_LOG_INFO << "Attempt to route to an existing tangle ["
                          << messageWrapperProtoHelper.getAccountHash().getHash(keto::common::StringEncoding::HEX) << "] for source ["
                          << messageWrapperProtoHelper.getSourceAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
        }
    }

    // look to see if the message account is for this server
    if (messageWrapperProtoHelper.getAccountHash() == keto::server_common::ServerInfo::getInstance()->getAccountHash()) {

        routeLocal(messageWrapperProtoHelper);
        // the result of the local routing
        keto::proto::MessageWrapperResponse response;
        response.set_success(true);
        response.set_result("local");
        return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
    }

    // check if we can rout to a peer
    keto::proto::RpcPeer rpcPeer;
    keto::asn1::HashHelper peerAccountHash = messageWrapperProtoHelper.getAccountHash();
    while (keto::router_db::RouterStore::getInstance()->getAccountRouting(
            peerAccountHash,rpcPeer)) {
        keto::router_utils::RpcPeerHelper rpcPeerHelper(
                rpcPeer);
        if (PeerCache::getInstance()->contains(rpcPeerHelper.getAccountHash())) {
            this->routeToRpcClient(messageWrapperProtoHelper,
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
    
    this->routeToRpcPeer(messageWrapperProtoHelper);
    
    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    response.set_result("to peer");
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}



keto::event::Event RouterService::registerRpcPeerClient(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper  rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    rpcPeerHelper.setPeerAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    PeerCache::getInstance()->addPeer(rpcPeerHelper);

    keto::router_db::RouterStore::getInstance()->persistPeerRouting(rpcPeerHelper);

    // push to peers
    keto::router_utils::RpcPeerHelper  parentRpcPeerHelper;
    parentRpcPeerHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    parentRpcPeerHelper.addChild(rpcPeerHelper);

    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ROUTER_QUERY::PUSH_RPC_PEER,parentRpcPeerHelper));

    return event;
}

keto::event::Event RouterService::registerRpcPeerServer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper  rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));

    PeerCache::getInstance()->addPeer(rpcPeerHelper);

    return event;
}



keto::event::Event RouterService::processPushRpcPeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper  rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));
    rpcPeerHelper.setPeerAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());

    keto::router_db::RouterStore::getInstance()->persistPeerRouting(rpcPeerHelper);

    // push up the tree
    keto::server_common::triggerEvent(
            keto::server_common::toEvent<keto::proto::RpcPeer>(
                    keto::server_common::Events::ROUTER_QUERY::PUSH_RPC_PEER,rpcPeerHelper));

    return event;
}

keto::event::Event RouterService::deregisterRpcPeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper  rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));

    PeerCache::getInstance()->removePeer(rpcPeerHelper);

    return event;
}

keto::event::Event RouterService::activateRpcPeer(const keto::event::Event& event) {
    keto::router_utils::RpcPeerHelper  rpcPeerHelper(
            keto::server_common::fromEvent<keto::proto::RpcPeer>(event));

    PeerCache::getInstance()->activateRpcPeer(rpcPeerHelper);
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
    keto::proto::Transaction transaction = *transactionProtoHelper;
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


void RouterService::routeLocal(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper) {
    keto::transaction_common::TransactionProtoHelperPtr transactionProtoHelperPtr = messageWrapperProtoHelper.getTransaction();
    if (transactionProtoHelperPtr->getStatus() == keto::proto::TransactionStatus::DEBIT ||
            transactionProtoHelperPtr->getStatus() == keto::proto::TransactionStatus::INIT) {
        if (messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_INIT ||
                messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapperProtoHelper.setOperation(keto::proto::MessageOperation::MESSAGE_BALANCE);

            // at present the only services registered are for the local service.
            // nothing is propigated. This means we are balancing locally on a single account
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapperProtoHelper.setAccountHash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapperProtoHelper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapperProtoHelper));
            }
        } else if (messageWrapperProtoHelper.getOperation() ==
                keto::proto::MessageOperation::MESSAGE_BALANCE) {
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapperProtoHelper));
        } else if (messageWrapperProtoHelper.getOperation() ==
                keto::proto::MessageOperation::MESSAGE_BLOCK) {
            
        } else if (messageWrapperProtoHelper.getOperation() ==
                keto::proto::MessageOperation::MESSAGE_PROCESS) {
            
        }
        
    } else if (transactionProtoHelperPtr->getStatus() == keto::proto::TransactionStatus::CREDIT) {
        if (messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_INIT ||
                messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapperProtoHelper.setOperation(keto::proto::MessageOperation::MESSAGE_BALANCE);

            // at present the only services registered are for the local service.
            // nothing is propigated. This means we are balancing locally on a single account
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapperProtoHelper.setAccountHash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapperProtoHelper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapperProtoHelper));
            }
        } else if (messageWrapperProtoHelper.getOperation() ==
                keto::proto::MessageOperation::MESSAGE_BALANCE) {
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapperProtoHelper));
        } else if (messageWrapperProtoHelper.getOperation() ==
                keto::proto::MessageOperation::MESSAGE_BLOCK) {
            // not implemented at present
        } else if (messageWrapperProtoHelper.getOperation() ==
                keto::proto::MessageOperation::MESSAGE_PROCESS) {
            // implememnted at present
        }
        
    }
}

void RouterService::routeToAccount(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper) {
    keto::transaction_common::TransactionProtoHelperPtr transactionProtoHelperPtr = messageWrapperProtoHelper.getTransaction();
    if (transactionProtoHelperPtr->getStatus() == keto::proto::TransactionStatus::DEBIT ||
            transactionProtoHelperPtr->getStatus() == keto::proto::TransactionStatus::INIT) {
        if (messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_INIT ||
                messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapperProtoHelper.setOperation(keto::proto::MessageOperation::MESSAGE_BALANCE);

            // at present the only services registered are for the local service.
            // nothing is propigated. This means we are balancing locally on a single account
            AccountHashVector accountHashVector =
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapperProtoHelper.setAccountHash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapperProtoHelper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapperProtoHelper));
            }
        }
        
    } else if (transactionProtoHelperPtr->getStatus() == keto::proto::TransactionStatus::CREDIT) {
        if (messageWrapperProtoHelper.getOperation() == keto::proto::MessageOperation::MESSAGE_ROUTE) {
            messageWrapperProtoHelper.setOperation(keto::proto::MessageOperation::MESSAGE_BALANCE);

            // at present the only services registered are for the local service.
            // nothing is propigated. This means we are balancing locally on a single account
            AccountHashVector accountHashVector = 
                    RouterRegistry::getInstance()->getAccount(
                    keto::server_common::Constants::SERVICE::BALANCE);
            messageWrapperProtoHelper.setAccountHash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
            if (RouterRegistry::getInstance()->isAccountLocal(accountHashVector)) {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::BALANCER_MESSAGE,messageWrapperProtoHelper));
            } else {
                keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapperProtoHelper));
            }
        }
    }
}


void RouterService::routeToRpcClient(keto::transaction_common::MessageWrapperProtoHelper&  messageWrapperProtoHelper,
        keto::router_utils::RpcPeerHelper& rpcPeerHelper) {
    
    if (!rpcPeerHelper.isServer()) {
        messageWrapperProtoHelper.setAccountHash(rpcPeerHelper.getAccountHashString());

        try {
            keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                            keto::server_common::Events::RPC_SERVER_TRANSACTION,messageWrapperProtoHelper));
        } catch (...) {
            // must place in a queue here
            KETO_LOG_INFO << "[RouterService] " << 
                    Botan::hex_encode((uint8_t*)rpcPeerHelper.getAccountHashString().data(),
                    rpcPeerHelper.getAccountHashString().size(),true)
                    << " Failed to dispatch to the server must add to a queue here";
        }
    } else {
        // route to a peer
        routeToRpcPeer(messageWrapperProtoHelper);
    }
}

void RouterService::routeToRpcPeer(keto::transaction_common::MessageWrapperProtoHelper& messageWrapperProtoHelper) {
    try {
        keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::server_common::Events::RPC_CLIENT_TRANSACTION,messageWrapperProtoHelper));
    } catch (...) {
        // must place in a queue here
        KETO_LOG_INFO << "[RouterService] Failed to dispatch to an approriate peer";
    }
}

}
}