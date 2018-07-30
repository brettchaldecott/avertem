/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Log.hpp
 * Author: ubuntu
 *
 * Created on January 13, 2018, 6:15 PM
 */

#ifndef LOG_MANAGER_HPP
#define LOG_MANAGER_HPP

#include <memory>

#include "keto/environment/Env.hpp"
#include "keto/environment/Config.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace environment {

class LogManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();

    LogManager(
            const std::shared_ptr<Env>& envPtr,
            const std::shared_ptr<Config>& configPtr);
    LogManager(const LogManager& orig) = delete;
    virtual ~LogManager();
private:
    std::shared_ptr<Env> envPtr;
    std::shared_ptr<Config> configPtr;

};


}
}
#endif /* LOG_HPP */

