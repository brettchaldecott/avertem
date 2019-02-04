/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.cpp
 * Author: ubuntu
 * 
 * Created on March 8, 2018, 3:15 AM
 */

#include "keto/block/EventRegistry.hpp"
#include "keto/block/BlockService.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/block/ConsensusService.hpp"
#include "keto/block/BlockProducer.hpp"

namespace keto {
namespace block {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}


void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::ENABLE_BLOCK_PRODUCER,
            &keto::block::EventRegistry::enableBlockProducer);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::BLOCK_MESSAGE,
            &keto::block::EventRegistry::blockMessage);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::BLOCK,
            &keto::block::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::BLOCK,
            &keto::block::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::BLOCK,
            &keto::block::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_STATE::BLOCK,
            &keto::block::EventRegistry::setupNodeConsensusSession);
}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::BLOCK);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::BLOCK);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::BLOCK);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_STATE::BLOCK);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::BLOCK_MESSAGE);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::ENABLE_BLOCK_PRODUCER);
}

keto::event::Event EventRegistry::blockMessage(const keto::event::Event& event) {
    return BlockService::getInstance()->blockMessage(event);
}


keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}

keto::event::Event EventRegistry::setupNodeConsensusSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setupNodeConsensusSession(event);
}

keto::event::Event EventRegistry::consensusSessionAccepted(const keto::event::Event& event) {
    return ConsensusService::getInstance()->consensusSessionAccepted(event);
}

keto::event::Event EventRegistry::enableBlockProducer(const keto::event::Event& event) {
    BlockProducer::getInstance()->setState(BlockProducer::State::block_producer);
    return event;
}

}
}