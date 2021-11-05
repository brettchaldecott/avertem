//
// Created by Brett Chaldecott on 2021/11/04.
//

#ifndef KETO_SANDBOXFORKMANAGER_HPP
#define KETO_SANDBOXFORKMANAGER_HPP


#include <string>
#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/sandbox/SandboxFork.hpp"

namespace keto {
namespace sandbox {

class SandboxForkManager;
typedef std::shared_ptr<SandboxForkManager> SandboxForkManagerPtr;

class SandboxForkManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class SandboxForkWrapper {
    public:
        SandboxForkWrapper(const SandboxForkPtr sandboxForkPtr);
        SandboxForkWrapper(const SandboxForkWrapper& orig) = delete;
        virtual ~SandboxForkWrapper();

        keto::event::Event executeActionMessage(const keto::event::Event& event);
        keto::event::Event executeHttpActionMessage(const keto::event::Event& event);
    private:
        SandboxForkPtr sandboxForkPtr;
    };
    typedef std::shared_ptr<SandboxForkWrapper> SandboxForkWrapperPtr;

    friend SandboxForkWrapper;

    SandboxForkManager();
    SandboxForkManager(const SandboxForkManager& orig) = delete;
    virtual ~SandboxForkManager();

    static SandboxForkManagerPtr init();
    static void fin();
    static SandboxForkManagerPtr getInstance();

    SandboxForkWrapperPtr getFork();
private:
    std::mutex classMutex;
    std::condition_variable stateCondition;
    int forkCount;
    std::deque<SandboxForkPtr> sandboxForkDequeu;

    void releaseFork(const SandboxForkPtr& sandboxForkPtr);
    void terminate();
};

}
}

#endif //KETO_SANDBOXFORKMANAGER_HPP
