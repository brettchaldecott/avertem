/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusServices.hpp
 * Author: ubuntu
 *
 * Created on July 18, 2018, 12:42 PM
 */

#ifndef CONSENSUSSERVICES_HPP
#define CONSENSUSSERVICES_HPP

#include <string>
#include <memory>

#include "keto/event/Event.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

namespace keto {
namespace consensus_module {

class ConsensusServices;
typedef std::shared_ptr<ConsensusServices> ConsensusServicesPtr;
    
class ConsensusServices {
public:
    ConsensusServices(const ConsensusServices& orig) = delete;
    virtual ~ConsensusServices();
    
    // account service management methods
    static ConsensusServicesPtr init(
            const keto::software_consensus::ConsensusHashGeneratorPtr& seedHashGenerator,
            const keto::software_consensus::ConsensusHashGeneratorPtr& moduleHashGenerator);
    static void fin();
    static ConsensusServicesPtr getInstance();
    
    // the event
    keto::event::Event generateSoftwareConsensus(const keto::event::Event& event);
    keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    keto::event::Event setModuleSession(const keto::event::Event& event);
    
private:
    keto::software_consensus::ConsensusHashGeneratorPtr seedHashGenerator;
    keto::software_consensus::ConsensusHashGeneratorPtr moduleHashGenerator;
    
    ConsensusServices(
            const keto::software_consensus::ConsensusHashGeneratorPtr& seedHashGenerator,
            const keto::software_consensus::ConsensusHashGeneratorPtr& moduleHashGenerator);
    

};


}
}


#endif /* CONSENSUSSERVICES_HPP */

