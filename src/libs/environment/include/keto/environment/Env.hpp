/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   evw.hpp
 * Author: ubuntu
 *
 * Created on January 10, 2018, 3:33 PM
 */

#ifndef ENV_HPP
#define ENV_HPP

#include <string>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace environment {

class Env {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();

    Env();
    virtual ~Env();
    Env(const Env& origin) = delete;

    boost::filesystem::path getInstallDir();

private:
    std::string installDir;
};

} 
}

#endif /* EVW_HPP */

