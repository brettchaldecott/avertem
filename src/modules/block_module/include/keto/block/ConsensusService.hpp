/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusService.hpp
 * Author: ubuntu
 *
 * Created on July 23, 2018, 11:35 AM
 */

#ifndef KETO_BLOCK_CONSENSUSSERVICE_HPP
#define KETO_BLOCK_CONSENSUSSERVICE_HPP

#include <string>
#include <memory>

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace block {

class ConsensusService;
typedef std::shared_ptr<ConsensusService> ConsensusServicePtr;

class ConsensusService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    ConsensusService(
            const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator);
    ConsensusService(const ConsensusService& orig) = delete;
    virtual ~ConsensusService();
    
    // account service management methods
    static ConsensusServicePtr init(
            const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator);
    static void fin();
    static ConsensusServicePtr getInstance();
    
    keto::event::Event generateSoftwareHash(const keto::event::Event& event);
    keto::event::Event setModuleSession(const keto::event::Event& event);
    keto::event::Event setupNodeConsensusSession(const keto::event::Event& event);
    keto::event::Event consensusSessionAccepted(const keto::event::Event& event);

private:
    
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator;

};

}
}

#endif /* CONSENSUSSERVICE_HPP */

