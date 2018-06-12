/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusBuilder.hpp
 * Author: ubuntu
 *
 * Created on May 28, 2018, 9:44 AM
 */

#ifndef CONSENSUSBUILDER_HPP
#define CONSENSUSBUILDER_HPP

#include <memory>
#include <vector>

#include "keto/software_consensus/ConsensusMessageHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ConsensusBuilder;
typedef std::shared_ptr<ConsensusBuilder> ConsensusBuilderPtr;
    
class ConsensusBuilder {
public:
    static std::string getVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();
    
    ConsensusBuilder(
        const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr);
    ConsensusBuilder(const ConsensusBuilder& orig) = delete;
    virtual ~ConsensusBuilder();
    
    ConsensusBuilder& buildConsensus();
    ConsensusMessageHelper getConsensus();
private:
    ConsensusMessageHelper consensusMessageHelper;
};


}
}


#endif /* CONSENSUSBUILDER_HPP */

