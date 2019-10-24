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

    /*struct RootResolver : WAVM::Runtime::Resolver
    {
        RootResolver(WAVM::Runtime::Resolver& inInnerResolver, WAVM::Runtime::Compartment* inCompartment)
        : innerResolver(inInnerResolver), compartment(inCompartment)
        {
        }

        virtual bool resolve(const std::string& moduleName,
                             const std::string& exportName,
                             WAVM::IR::ExternType type,
                             WAVM::Runtime::Object*& outObject) override
        {
            if(innerResolver.resolve(moduleName, exportName, type, outObject)) { return true; }
            else
            {
                return generateStub(
                        moduleName, exportName, type, outObject, compartment, StubFunctionBehavior::trap);
            }
        }

    private:
        WAVM::Runtime::Resolver& innerResolver;
        GCPointer<WAVM::Runtime::Compartment> compartment;


        Compartment* compartment;
        std::map<std::string,Instance*> moduleNameToInstanceMap;

        RootResolver(Compartment* inCompartment): compartment(inCompartment) {}

        bool resolve(const std::string& moduleName,const std::string& exportName,WAVM::IR::ExternType type,Object*& outObject) override
        {
            auto namedInstanceIt = moduleNameToInstanceMap.find(moduleName);
            if(namedInstanceIt != moduleNameToInstanceMap.end())
            {
                outObject = getInstanceExport(namedInstanceIt->second,exportName);
                if(outObject)
                {
                    if(isA(outObject,type)) { return true; }
                    else
                    {
                        KETO_LOG_ERROR << "Resolved import " << moduleName << "." << exportName << " to a " << asString(getObjectType(outObject))
                            << " but was expecting " << asString(type);
                        return false;
                    }
                }
            }

            KETO_LOG_ERROR << "Generated stub for missing import " << moduleName << "." << moduleName << " : " << asString(type);
            outObject = getStubObject(type);
            return true;
        }

        Object* getStubObject(ObjectType type) const
        {
            // If the import couldn't be resolved, stub it in.
            switch(type.kind)
            {
                case IR::ObjectKind::function:
                {
                    // Generate a function body that just uses the unreachable op to fault if called.
                    Serialization::ArrayOutputStream codeStream;
                    OperatorEncoderStream encoder(codeStream);
                    encoder.unreachable();
                    encoder.end();

                    // Generate a module for the stub function.
                    Module stubModule;
                    DisassemblyNames stubModuleNames;
                    stubModule.types.push_back(asFunctionType(type));
                    stubModule.functions.defs.push_back({{0},{},std::move(codeStream.getBytes()),{}});
                    stubModule.exports.push_back({"importStub",IR::ObjectKind::function,0});
                    stubModuleNames.functions.push_back({"importStub <" + asString(type) + ">",{},{}});
                    IR::setDisassemblyNames(stubModule,stubModuleNames);
                    IR::validateDefinitions(stubModule);

                    // Instantiate the module and return the stub function instance.
                    auto stubInstance = instantiateModule(compartment,stubModule,{});
                    return getInstanceExport(stubInstance,"importStub");
                }
                case IR::ObjectKind::memory:
                {
                    return asObject(Runtime::createMemory(compartment,asMemoryType(type)));
                }
                case IR::ObjectKind::table:
                {
                    return asObject(Runtime::createTable(compartment,asTableType(type)));
                }
                case IR::ObjectKind::global:
                {
                    return asObject(Runtime::createGlobal(
                            compartment,
                            asGlobalType(type),
                            Runtime::Value(asGlobalType(type).valueType,Runtime::UntaggedValue())));
                }
                case IR::ObjectKind::exceptionType:
                {
                    return asObject(Runtime::createExceptionTypeInstance(asExceptionTypeType(type)));
                }
                default: {
                    KETO_LOG_ERROR << "Reached the unreachable";

                    Errors::unreachable();
                }
            };
        }
    };*/


static WavmEngineManagerPtr singleton;

//WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment>* compartment;

std::string WavmEngineManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

WavmEngineManager::WavmEngineManager() {
    //compartment = new Runtime::GCPointer<Runtime::Compartment>();
    //*compartment = Runtime::createCompartment();
    //this->emscriptenInstance = keto::Emscripten::instantiate(*compartment);

    //RootResolver* rootResolver = new RootResolver(*compartment);

    KETO_LOG_DEBUG << "After getting the instance.";
    //rootResolver->moduleNameToInstanceMap["env"] = emscriptenInstance->env;
    //rootResolver->moduleNameToInstanceMap["asm2wasm"] = emscriptenInstance->asm2wasm;
    //rootResolver->moduleNameToInstanceMap["global"] = emscriptenInstance->global;
    //rootResolver->moduleNameToInstanceMap["Keto"] = emscriptenInstance->keto;
    //rootResolver->moduleNameToInstanceMap["keto"] = emscriptenInstance->keto;

    //this->resolver = rootResolver;

}

WavmEngineManager::~WavmEngineManager() {
    //delete compartment;
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


//Runtime::Compartment* WavmEngineManager::getCompartment() {
//    return *compartment;
//}

//keto::Emscripten::Instance* WavmEngineManager::getEmscriptenInstance() {
//    return this->emscriptenInstance;
//}

//Runtime::Resolver* WavmEngineManager::getResolver() {
//    return this->resolver;
//}

}
}
