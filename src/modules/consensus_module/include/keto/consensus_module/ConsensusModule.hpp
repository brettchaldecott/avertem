/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusModule.hpp
 * Author: ubuntu
 *
 * Created on July 18, 2018, 7:16 AM
 */

#ifndef KETO_CONSENSUSMODULE_HPP
#define KETO_CONSENSUSMODULE_HPP

#include <string>
#include <map>
#include <memory>

#include "keto/module/ModuleInterface.hpp"

namespace keto {
namespace consensus_module {

class ConsensusModule : public keto::module::ModuleInterface {
public:
    ConsensusModule();
    ConsensusModule(const ConsensusModule& orig) = delete;
    virtual ~ConsensusModule();
    
    // meta methods
    virtual const std::string getName() const;
    virtual const std::string getDescription() const;
    virtual const std::string getVersion() const;

    
private:

};

}
}

#endif /* CONSENSUSMODULE_HPP */

