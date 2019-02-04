//
// Created by Brett Chaldecott on 2019/02/04.
//

#include "keto/software_consensus/ConsensusStateManager.hpp"


namespace keto {
namespace software_consensus {

static ConsensusStateManagerPtr singleton;


ConsensusStateManager::~ConsensusStateManager() {

}


ConsensusStateManagerPtr ConsensusStateManager::init() {
    return singleton = ConsensusStateManagerPtr(new ConsensusStateManager());
}

ConsensusStateManagerPtr ConsensusStateManager::getInstance() {
    return singleton;
}

void ConsensusStateManager::fin() {
    singleton.reset();
}

ConsensusStateManager::State ConsensusStateManager::getState() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->currentState;
}

void ConsensusStateManager::setState(ConsensusStateManager::State state) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    this->currentState = state;
}


ConsensusStateManager::ConsensusStateManager() {
    this->currentState = State::INIT;
}

}
}