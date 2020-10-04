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

StateMonitor::StateMonitor() : active(false), deactivateTime(std::chrono::system_clock::now()) {
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
            (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - this->deactivateTime).count() < 120)) {
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
    this->deactivateTime = std::chrono::system_clock::now();
    this->stateCondition.notify_all();
}

}
}
