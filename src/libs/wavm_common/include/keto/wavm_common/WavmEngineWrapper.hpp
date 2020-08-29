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

//#include "WAVM/Runtime/Runtime.h"

#include "keto/wavm_common/Emscripten.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace Runtime {
    struct Compartment;
    struct Context;
}

namespace keto {
namespace wavm_common {

class WavmEngineManager;
class WavmEngineWrapper;
typedef std::shared_ptr<WavmEngineWrapper> WavmEngineWrapperPtr;

class WavmEngineWrapper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    WavmEngineWrapper(const std::string& wast, const std::string& contract);
    WavmEngineWrapper(const WavmEngineWrapper& orig) = delete;
    virtual ~WavmEngineWrapper();
    
    void execute();
    void executeHttp();
    
private:
    //WavmEngineManager& wavmEngineManager;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment;
    std::shared_ptr<keto::Emscripten::Process> emscriptenProcess;
    std::string wast;
    std::string contract;


    void internalExecute();
    void internalExecuteHttp();

};


}
}


#endif /* WAVMENGINEWRAPPER_HPP */

