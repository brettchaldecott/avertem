/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdlib>
#include "keto/environment/Env.hpp"
#include "keto/environment/Exception.hpp"
#include "keto/environment/Constants.hpp"

namespace keto {
namespace environment {

std::string Env::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

Env::Env() {
    if (!std::getenv(Constants::KETO_HOME)) {
        BOOST_THROW_EXCEPTION(keto::environment::HomeNotSet());
    }
    this->installDir = std::getenv(Constants::KETO_HOME);
    
}


Env::~Env(){
    
}


boost::filesystem::path Env::getInstallDir() {
    return this->installDir;
}

}
}
