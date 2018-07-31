/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmEngineWrapper.hpp
 * Author: ubuntu
 *
 * Created on April 9, 2018, 8:46 AM
 */

#ifndef WAVMENGINEWRAPPER_HPP
#define WAVMENGINEWRAPPER_HPP

#include <string>
#include <memory>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace wavm_common {

class WavmEngineWrapper;
typedef std::shared_ptr<WavmEngineWrapper> WavmEngineWrapperPtr;

class WavmEngineWrapper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    WavmEngineWrapper(const std::string& wast);
    WavmEngineWrapper(const WavmEngineWrapper& orig) = delete;
    virtual ~WavmEngineWrapper();
    
    void execute();
    
private:
    std::string wast;
    
};


}
}


#endif /* WAVMENGINEWRAPPER_HPP */

