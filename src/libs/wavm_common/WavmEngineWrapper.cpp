/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmEngineWrapper.cpp
 * Author: ubuntu
 * 
 * Created on April 9, 2018, 8:46 AM
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

//#include "Programs/CLI.h"
#include "Platform/Platform.h"
#include "Inline/BasicTypes.h"
#include "Inline/Floats.h"
#include "Inline/Timing.h"
#include "keto/wavm_common/Emscripten.hpp"
#include "Inline/BasicTypes.h"
#include "Inline/Timing.h"
#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"
#include "Runtime/Runtime.h"
#include "Runtime/RuntimePrivate.h"
#include "ThreadTest/ThreadTest.h"
#include "WAST/WAST.h"
#include "WASM/WASM.h"

#include "keto/wavm_common/WavmEngineWrapper.hpp"
#include "keto/wavm_common/Exception.hpp"
#include "keto/wavm_common/WavmSession.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"
#include "keto/wavm_common/WavmEngineManager.hpp"



using namespace IR;
using namespace Runtime;



namespace keto {
namespace wavm_common {


bool loadBinaryModule(const std::string& wasmBytes,IR::Module& outModule)
{
    // Load the module from a binary WebAssembly file.
    try
    {
        Serialization::MemoryInputStream stream((const U8*)wasmBytes.data(),wasmBytes.size());
        WASM::serialize(stream,outModule);
    }
    catch(Serialization::FatalSerializationException exception)
    {
        KETO_LOG_ERROR <<  "[WavmEngineWrapper][loadBinaryModule]Error deserializing WebAssembly binary file:";
        KETO_LOG_ERROR <<  exception.message << std::endl;
        return false;
    }
    catch(IR::ValidationException exception)
    {
        KETO_LOG_ERROR <<  "[WavmEngineWrapper][loadBinaryModule]Error validating WebAssembly binary file:";
        KETO_LOG_ERROR <<  exception.message;
        return false;
    }
    catch(std::bad_alloc)
    {
        KETO_LOG_ERROR <<  "[WavmEngineWrapper][loadBinaryModule]Memory allocation failed: input is likely malformed";
        return false;
    }

    return true;
}
    
bool loadTextModule(const std::string& wastString,IR::Module& outModule)
{
    std::vector<WAST::Error> parseErrors;
    WAST::parseModule(wastString.c_str(),wastString.size(),outModule,parseErrors);
    if(!parseErrors.size()) {
        return true; 
    }
    return false;
}


std::string WavmEngineWrapper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

WavmEngineWrapper::WavmEngineWrapper(WavmEngineManager& wavmEngineManager, const std::string& wast)
    : wavmEngineManager(wavmEngineManager), wast(wast) {
}


WavmEngineWrapper::~WavmEngineWrapper() {

}



void WavmEngineWrapper::execute() {
    Module module;

    // Enable some additional "features" in WAVM that are disabled by default.
    module.featureSpec.importExportMutableGlobals = true;
    module.featureSpec.sharedTables = true;
    // Allow atomics on unshared memories to accomodate atomics on the Emscripten memory.
    module.featureSpec.requireSharedFlagForAtomicOperators = false;

    // check if we are dealing with a binary contract or not
    if(*(U32*)wast.data() == 0x6d736100) {
        if (!loadBinaryModule(wast,module)) {
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidContractException());
        }
    } else if(!loadTextModule(wast,module)) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidContractException());
    }

    // Link the module with the intrinsic modules.
    Compartment* compartment = Runtime::cloneCompartment(
            this->wavmEngineManager.getCompartment());
    Context* context = Runtime::createContext(compartment);
    try {


        LinkResult linkResult = linkModule(module, *this->wavmEngineManager.getResolver());
        if (!linkResult.success) {
            std::stringstream ss;
            ss << "Failed to link module:" << std::endl;
            for (auto &missingImport : linkResult.missingImports) {
                ss << "Missing import: module=\"" << missingImport.moduleName
                   << "\" export=\"" << missingImport.exportName
                   << "\" type=\"" << asString(missingImport.type) << "\"" << std::endl;
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::LinkingFailedException(ss.str()));
        }

        // Instantiate the module.
        ModuleInstance *moduleInstance = instantiateModule(compartment, module,
                                                           std::move(linkResult.resolvedImports));
        if (!moduleInstance) { BOOST_THROW_EXCEPTION(keto::wavm_common::LinkingFailedException()); }

        // Call the module start function, if it has one.
        FunctionInstance *startFunction = getStartFunction(moduleInstance);
        if (startFunction) {
            invokeFunctionChecked(context, startFunction, {});
        }

        keto::Emscripten::initializeGlobals(context, module, moduleInstance);

        // Look up the function export to call.
        FunctionInstance *functionInstance;
        Status currentStatus = WavmSessionManager::getInstance()->getWavmSession()->getStatus();
        if ((currentStatus == Status_init) ||
            (currentStatus == Status_debit)) {
            functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "debit"));
        } else if (currentStatus == Status_credit) {
            functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "credit"));
        } else if (currentStatus == Status_processing) {
            functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "process"));
        } else {
            BOOST_THROW_EXCEPTION(keto::wavm_common::UnsupportedInvocationStatusException());
        }

        if (!functionInstance) {
            functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "_main"));
        }
        if (!functionInstance) {
            BOOST_THROW_EXCEPTION(keto::wavm_common::MissingEntryPointException());
        }
        const FunctionType *functionType = getFunctionType(functionInstance);

        // Set up the arguments for the invoke.
        std::vector<Value> invokeArgs;
        //Timing::Timer executionTimer;
        Runtime::catchRuntimeExceptions(
                [&] {
                    // invoke the function
                    invokeFunctionChecked(context, functionInstance, invokeArgs);

                    // collect the garbage
                    Runtime::collectGarbage();
                },
                [&](Runtime::Exception &&exception) {
                    std::stringstream ss;
                    ss << "Failed to handle the exception : " << describeException(exception).c_str();

                    // collect the garbage
                    try {
                        Runtime::collectGarbage();
                    } catch (...) {
                        KETO_LOG_INFO << "[WavmEngineWrapper][execute] failed to garbage collect";
                    }

                    std::cout << ss.str() << std::endl;
                    BOOST_THROW_EXCEPTION(keto::wavm_common::ContactExecutionFailedException(ss.str()));
                });
        // free memory
        std::cout << "function instance" << std::endl;
        Runtime::removeGCRoot((Object*)functionInstance);
        std::cout << "release the module instance" << std::endl;
        Runtime::removeGCRoot((Object*)moduleInstance);
        std::cout << "release the context" << std::endl;
        Runtime::removeGCRoot((Object*)context);
        std::cout << "remove the compartment" << std::endl;
        Runtime::removeGCRoot((Object*)compartment);
        std::cout << "collect the garbage" << std::endl;
        Runtime::collectGarbage();
        std::cout << "after collect garbage" << std::endl;
        //delete emscriptenInstance;
        //std::cout << "delete the emscript" << std::endl;
        //delete moduleInstance;

    } catch (...) {

        //context->finalize();
        //Runtime::removeGCRoot((Object*)context);
        //Runtime::removeGCRoot((Object*)compartment);
        Runtime::collectGarbage();
        throw;
    }

}

}
}
