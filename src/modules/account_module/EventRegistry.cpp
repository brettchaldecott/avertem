/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventRegistry.cpp
 * Author: ubuntu
 * 
 * Created on March 6, 2018, 1:42 PM
 */

#include "keto/account/EventRegistry.hpp"

#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"


#include "keto/software_consensus/ConsensusStateManager.hpp"
#include "keto/account/AccountService.hpp"
#include "keto/account/ConsensusService.hpp"

namespace keto {
namespace account {

std::string EventRegistry::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
EventRegistry::EventRegistry() {
}

EventRegistry::~EventRegistry() {
}

keto::event::Event EventRegistry::checkAccount(const keto::event::Event& event) {
    return AccountService::getInstance()->checkAccount(event);
}

keto::event::Event EventRegistry::applyDirtyTransaction(const keto::event::Event& event) {
    return AccountService::getInstance()->applyDirtyTransaction(event);
}


keto::event::Event EventRegistry::applyTransaction(const keto::event::Event& event) {
    return AccountService::getInstance()->applyTransaction(event);
}

keto::event::Event EventRegistry::sparqlQuery(const keto::event::Event& event) {
    return AccountService::getInstance()->sparqlQuery(event);
}

keto::event::Event EventRegistry::dirtySparqlQueryWithResultSet(const keto::event::Event& event) {
    return AccountService::getInstance()->dirtySparqlQueryWithResultSet(event);
}

keto::event::Event EventRegistry::sparqlQueryWithResultSet(const keto::event::Event& event) {
    return AccountService::getInstance()->sparqlQueryWithResultSet(event);
}


keto::event::Event EventRegistry::getContract(const keto::event::Event& event) {
    return AccountService::getInstance()->getContract(event);
}

keto::event::Event EventRegistry::clearDirty(const keto::event::Event& event) {
    return AccountService::getInstance()->clearDirty(event);
}

keto::event::Event EventRegistry::generateSoftwareHash(const keto::event::Event& event) {
    return ConsensusService::getInstance()->generateSoftwareHash(event);
}

keto::event::Event EventRegistry::setModuleSession(const keto::event::Event& event) {
    return ConsensusService::getInstance()->setModuleSession(event);
}

keto::event::Event EventRegistry::consensusSessionAccepted(const keto::event::Event& event) {
    keto::software_consensus::ConsensusStateManager::getInstance()->setState(
            keto::software_consensus::ConsensusStateManager::ACCEPTED);
    return event;
}

keto::event::Event EventRegistry::consensusProtocolCheck(const keto::event::Event& event) {

    return event;
}

keto::event::Event EventRegistry::consensusHeartbeat(const keto::event::Event& event) {

    return event;
}

void EventRegistry::registerEventHandlers() {
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CHECK_ACCOUNT_MESSAGE,
            &keto::account::EventRegistry::checkAccount);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::APPLY_ACCOUNT_DIRTY_TRANSACTION_MESSAGE,
            &keto::account::EventRegistry::applyDirtyTransaction);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE,
            &keto::account::EventRegistry::applyTransaction);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::SPARQL_QUERY_MESSAGE,
            &keto::account::EventRegistry::sparqlQuery);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
            &keto::account::EventRegistry::dirtySparqlQueryWithResultSet);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
            &keto::account::EventRegistry::sparqlQueryWithResultSet);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::GET_CONTRACT,
            &keto::account::EventRegistry::getContract);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CLEAR_DIRTY_CACHE,
            &keto::account::EventRegistry::clearDirty);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS::ACCOUNT,
            &keto::account::EventRegistry::generateSoftwareHash);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::ACCOUNT,
            &keto::account::EventRegistry::setModuleSession);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::ACCOUNT,
            &keto::account::EventRegistry::consensusSessionAccepted);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::ACCOUNT,
            &keto::account::EventRegistry::consensusProtocolCheck);
    keto::server_common::registerEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::ACCOUNT,
            &keto::account::EventRegistry::consensusHeartbeat);
}

void EventRegistry::deregisterEventHandlers() {
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_HEARTBEAT::ACCOUNT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_CHECK::ACCOUNT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION_ACCEPTED::ACCOUNT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS_SESSION::ACCOUNT);
    keto::server_common::deregisterEventHandler (
            keto::server_common::Events::CONSENSUS::ACCOUNT);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CLEAR_DIRTY_CACHE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::GET_CONTRACT);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SPARQL_QUERY_WITH_RESULTSET_MESSAGE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::DIRTY_SPARQL_QUERY_WITH_RESULTSET_MESSAGE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::SPARQL_QUERY_MESSAGE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::APPLY_ACCOUNT_DIRTY_TRANSACTION_MESSAGE);
    keto::server_common::deregisterEventHandler(
            keto::server_common::Events::CHECK_ACCOUNT_MESSAGE);
}


}
}
