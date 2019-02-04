//
// Created by Brett Chaldecott on 2019/02/04.
//

#ifndef KETO_CONSENSUSSTATEMANAGER_HPP
#define KETO_CONSENSUSSTATEMANAGER_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace keto {
namespace software_consensus {

class ConsensusStateManager;
typedef std::shared_ptr<ConsensusStateManager> ConsensusStateManagerPtr;

class ConsensusStateManager {
public:
    enum State {
        INIT,
        GENERATE,
        ACCEPTED,
        REJECTED,
        FIN
    };

    ConsensusStateManager(const ConsensusStateManager& orig) = delete;
    virtual ~ConsensusStateManager();


    static ConsensusStateManagerPtr init();
    static ConsensusStateManagerPtr getInstance();
    static void fin();

    State getState();
    void setState(State state);

private:
    std::mutex classMutex;
    State currentState;

    ConsensusStateManager();

};

}
}


#endif //KETO_CONSENSUSSTATEMANAGER_HPP
