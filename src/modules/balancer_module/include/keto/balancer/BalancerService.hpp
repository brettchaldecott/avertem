/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BalancerService.hpp
 * Author: ubuntu
 *
 * Created on March 31, 2018, 9:50 AM
 */

#ifndef BALANCERSERVICE_HPP
#define BALANCERSERVICE_HPP

#include <string>
#include <memory>
#include <mutex>

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/server_common/StatePersistanceManager.hpp"

namespace keto {
namespace balancer {

class BalancerService;
typedef std::shared_ptr<BalancerService> BalancerServicePtr;

class BalancerService {
public:
    /**
     * The state enum containing the various states that the module manager can
     * be in.
     */
    enum State {
        inited,
        active_balancer,
        active_balancer_complete,
        terminated,
        inactive_balancer
    };

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    BalancerService();
    BalancerService(const BalancerService& orig) = delete;
    virtual ~BalancerService();
    
    static BalancerServicePtr init();
    static void fin();
    static BalancerServicePtr getInstance();

    void setState(const State& state);
    State getState();

    keto::event::Event balanceMessage(const keto::event::Event& event);
    keto::event::Event consensusHeartbeat(const keto::event::Event& event);
    
private:
    std::mutex classMutex;
    State currentState;


};


}
}

#endif /* BALANCERSERVICE_HPP */

