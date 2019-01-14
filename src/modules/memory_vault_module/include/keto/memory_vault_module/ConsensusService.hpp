//
// Created by Brett Chaldecott on 2019/01/14.
//

#ifndef KETO_MEMORY_VAULT_CONSENSUSSERVICE_HPP
#define KETO_MEMORY_VAULT_CONSENSUSSERVICE_HPP


#include <string>
#include <memory>

#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace memory_vault_module {

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

private:
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator;
};


}
}


#endif //KETO_CONSENSUSSERVICE_HPP
