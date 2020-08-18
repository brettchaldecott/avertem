/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmEngineManager.cpp
 * Author: ubuntu
 * 
 * Created on April 9, 2018, 9:12 AM
 */

#include <condition_variable>

#include "keto/wavm_common/Emscripten.hpp"

#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/Timing.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/Timing.h"
#include "WAVM/IR/Module.h"
#include "WAVM/IR/Operators.h"
#include "WAVM/IR/Validate.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/Runtime/Runtime.h"
#include "WAVM/ThreadTest/ThreadTest.h"

#include "keto/wavm_common/WavmEngineManager.hpp"
#include "keto/wavm_common/WavmEngineWrapper.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"


using namespace WAVM::IR;
using namespace WAVM::Runtime;

namespace keto {
namespace wavm_common {

static WavmEngineManagerPtr singleton;

std::string WavmEngineManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

WavmEngineManager::WavmEngineManager() {
}

WavmEngineManager::~WavmEngineManager() {
}

WavmEngineManagerPtr WavmEngineManager::init() {
    singleton = std::make_shared<WavmEngineManager>();
    WavmSessionManager::init();
    return singleton;
}


void WavmEngineManager::fin() {
    WavmSessionManager::fin();
    singleton.reset();
}

WavmEngineManagerPtr WavmEngineManager::getInstance() {
    return singleton;
}


WavmEngineWrapperPtr WavmEngineManager::getEngine(const std::string& wast, const std::string& contract) {
    return WavmEngineWrapperPtr(new WavmEngineWrapper(wast, contract));
}

}
}
