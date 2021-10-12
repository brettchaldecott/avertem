//
// Created by Brett Chaldecott on 2020/08/07.
//

#include "keto/module/StateMonitor.hpp"


namespace keto {
namespace module {

static std::mutex stateMonitorMutex;
static SingletonStateMonitorPtr singletonStateMonitorPtr;

std::string StateMonitor::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

StateMonitor::StateMonitor() : active(false), deactivateTime(0) {
}

StateMonitor::~StateMonitor() {

}

SingletonStateMonitorPtr StateMonitor::getInstance() {
    std::lock_guard<std::mutex> guard(stateMonitorMutex);
    return singletonStateMonitorPtr;
}

SingletonStateMonitorPtr StateMonitor::init() {
    std::lock_guard<std::mutex> guard(stateMonitorMutex);
    return singletonStateMonitorPtr = SingletonStateMonitorPtr(new StateMonitor());
}

void StateMonitor::fin() {
    std::lock_guard<std::mutex> guard(stateMonitorMutex);
    singletonStateMonitorPtr.reset();
}

void StateMonitor::monitor() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    while(active ||
            ((time(0) - this->deactivateTime) < 120)) {
        this->stateCondition.wait_for(uniqueLock,std::chrono::milliseconds(30 * 1000));
    }
}

void StateMonitor::activate() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->active = true;
}

void StateMonitor::deactivate() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->active = false;
    this->deactivateTime = time(0);
    this->stateCondition.notify_all();
}

bool StateMonitor::isActive() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return this->active;
}

}
}
