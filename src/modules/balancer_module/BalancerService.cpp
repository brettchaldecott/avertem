/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BalancerService.cpp
 * Author: ubuntu
 * 
 * Created on March 31, 2018, 9:50 AM
 */

#include <condition_variable>
#include <iostream>

#include "Protocol.pb.h"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/StatePersistanceManager.hpp"

#include "keto/balancer/BalancerService.hpp"
#include "keto/balancer/BlockRouting.hpp"
#include "keto/balancer/BlockRouting.hpp"
#include "keto/balancer/Constants.hpp"

#include "keto/server_common/StatePersistanceManager.hpp"
#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"

namespace keto {
namespace balancer {

static BalancerServicePtr singleton;

std::string BalancerService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BalancerService::BalancerService() : currentState(BalancerService::State::unloaded) {

    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();

    if (config->getVariablesMap().count(Constants::STATE_STORAGE_CONFIG)) {
        keto::server_common::StatePersistanceManager::init(config->getVariablesMap()[Constants::STATE_STORAGE_CONFIG].as<std::string>());
    } else {
        keto::server_common::StatePersistanceManager::init(Constants::STATE_STORAGE_DEFAULT);
    }
}

BalancerService::~BalancerService() {
    keto::server_common::StatePersistanceManager::fin();
}

BalancerServicePtr BalancerService::init() {
    return singleton = std::make_shared<BalancerService>();
}

void BalancerService::fin() {
    singleton.reset();
}

BalancerServicePtr BalancerService::getInstance() {
    return singleton;
}

void BalancerService::setState(const BalancerService::State& state) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    _setState(state);
}

BalancerService::State BalancerService::getState() {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    return currentState;
}

void BalancerService::loadState(const BalancerService::State& state) {
    std::lock_guard<std::mutex> uniqueLock(this->classMutex);
    if (this->currentState == BalancerService::State::unloaded) {
        keto::server_common::StatePersistanceManagerPtr statePersistanceManagerPtr =
                keto::server_common::StatePersistanceManager::getInstance();
        if (statePersistanceManagerPtr->contains(Constants::PERSISTED_STATE)) {
            this->currentState = (BalancerService::State)(long)(*statePersistanceManagerPtr)[Constants::PERSISTED_STATE];
        } else {
            this->currentState = BalancerService::State::inited;
            _setState(state);
        }
    } else {
        _setState(state);
    }
}

void BalancerService::_setState(const BalancerService::State& state) {
    keto::server_common::StatePersistanceManagerPtr statePersistanceManagerPtr =
            keto::server_common::StatePersistanceManager::getInstance();
    keto::server_common::StatePersistanceManager::StateMonitorPtr stateMonitorPtr =
            statePersistanceManagerPtr->createStateMonitor();

    this->currentState = state;
    if (currentState != State::terminated) {
        (*statePersistanceManagerPtr)[Constants::PERSISTED_STATE].set((long) this->currentState);
    }
}

keto::event::Event BalancerService::balanceMessage(const keto::event::Event& event) {

    // at present
    KETO_LOG_DEBUG << "[BalancerService::balanceMessage] balance the message";
    keto::proto::MessageWrapper  messageWrapper =
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event);
    messageWrapper.set_message_operation(keto::proto::MessageOperation::MESSAGE_BLOCK);
    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
            keto::server_common::Events::BLOCK_MESSAGE,messageWrapper));

    // at present this is not implemented as originally praposed
    // this will change as the network requirements grow
    //
    //AccountHashVector accountHashVector = BlockRouting::getInstance()->getBlockAccount(
    //        keto::server_common::VectorUtils().copyStringToVector(messageWrapper.account_hash()));
    //messageWrapper.set_account_hash(keto::server_common::VectorUtils().copyVectorToString(accountHashVector));
    //if (accountHashVector == keto::server_common::ServerInfo::getInstance()->getAccountHash()) {
    //    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
    //                keto::server_common::Events::BLOCK_MESSAGE,messageWrapper));
    //} else {
    //    keto::server_common::triggerEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
    //                keto::server_common::Events::RPC_SEND_MESSAGE,messageWrapper));
    //}

    keto::proto::MessageWrapperResponse response;
    response.set_success(true);
    response.set_result("balanced");
    return keto::server_common::toEvent<keto::proto::MessageWrapperResponse>(response);
}


keto::event::Event BalancerService::consensusHeartbeat(const keto::event::Event& event) {

    KETO_LOG_DEBUG << "[BalancerService][consensusHeartbeat] balance [ not available yet ]";
    return event;
}

}
}