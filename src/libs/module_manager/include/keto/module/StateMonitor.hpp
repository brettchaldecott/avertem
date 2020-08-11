//
// Created by Brett Chaldecott on 2020/08/07.
//

#ifndef KETO_STATEMONITOR_HPP
#define KETO_STATEMONITOR_HPP

#include <memory>
#include <string>
#include <set>
#include <map>
#include <mutex>
#include <condition_variable>

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace module {

class StateMonitor;
typedef std::shared_ptr<StateMonitor> SingletonStateMonitorPtr;

class StateMonitor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    StateMonitor();
    StateMonitor(const StateMonitor& stateMonitor) = default;
    virtual ~StateMonitor();

    static SingletonStateMonitorPtr getInstance();
    static SingletonStateMonitorPtr init();
    static void fin();

    void monitor();
    void activate();
    void deactivate();

private:
    std::mutex classMutex;
    std::condition_variable stateCondition;
    bool active;

};

}
}

#endif //KETO_STATEMONITOR_HPP
