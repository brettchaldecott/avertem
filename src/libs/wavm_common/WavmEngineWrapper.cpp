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
#include "ThreadTest/ThreadTest.h"
#include "WAST/WAST.h"
#include "WASM/WASM.h"
#include "IR/Types.h"

#include "keto/wavm_common/WavmEngineWrapper.hpp"
#include "keto/wavm_common/Exception.hpp"
#include "keto/wavm_common/WavmSession.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"
#include "keto/wavm_common/WavmEngineManager.hpp"
#include "keto/wavm_common/WavmSessionTransaction.hpp"



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
        KETO_LOG_ERROR <<  exception.message;
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
    // place object in a scope
    try {
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
        Runtime::GCPointer<Compartment> compartment = Runtime::cloneCompartment(
                this->wavmEngineManager.getCompartment());
        Runtime::GCPointer<Context> context = Runtime::createContext(compartment);


        LinkResult linkResult = linkModule(module, *this->wavmEngineManager.getResolver());
        if (!linkResult.success) {
            std::stringstream ss;
            ss << "Failed to link module:";
            for (auto &missingImport : linkResult.missingImports) {
                ss << "Missing import: module=\"" << missingImport.moduleName
                   << "\" export=\"" << missingImport.exportName
                   << "\" type=\"" << asString(missingImport.type) << "\"";
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::LinkingFailedException(ss.str()));
        }

        Runtime::GCPointer<ModuleInstance> moduleInstance =
                instantiateModule(compartment, module, std::move(linkResult.resolvedImports));
        if (!moduleInstance) { BOOST_THROW_EXCEPTION(keto::wavm_common::LinkingFailedException()); }

        // Call the module start function, if it has one.
        Runtime::GCPointer<FunctionInstance> startFunction = getStartFunction(moduleInstance);
        if (startFunction) {
            invokeFunctionChecked(context, startFunction, {});
        }

        keto::Emscripten::initializeGlobals(context, module, moduleInstance);

        // Look up the function export to call.
        Runtime::GCPointer<FunctionInstance> functionInstance;
        Status currentStatus = std::dynamic_pointer_cast<WavmSessionTransaction>(WavmSessionManager::getInstance()->getWavmSession())->getStatus();
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
        //const FunctionType *functionType = getFunctionType(functionInstance);

        // Set up the arguments for the invoke.
        std::vector<Value> invokeArgs;
        Result functionResult;
        //Timing::Timer executionTimer;
        Runtime::catchRuntimeExceptions(
                [&] {
                    // invoke the function
                    functionResult = invokeFunctionChecked(context, functionInstance, invokeArgs);
                },
                [&](Runtime::Exception &&exception) {
                    std::stringstream ss;
                    ss << "Failed to handle the exception : " << describeException(exception).c_str();

                    KETO_LOG_DEBUG << ss.str();
                    BOOST_THROW_EXCEPTION(keto::wavm_common::ContactExecutionFailedException(ss.str()));
                });

        // validate the result
        if ((currentStatus == Status_init) ||
            (currentStatus == Status_debit)) {
            bool result = false;
            switch (functionResult.type) {
                case IR::ResultType::i32:
                    result = (functionResult.i32);
                    break;
                case IR::ResultType::i64:
                    result = (functionResult.i64);
                    break;
                case IR::ResultType::f32:
                    result = (functionResult.f32);
                    break;
                case IR::ResultType::f64:
                    result = (functionResult.f64);
                    break;
                default:
                    BOOST_THROW_EXCEPTION(keto::wavm_common::ContractUnsupportedResultException(
                            "Contract returned an unsupported result."));
            }
            if (!result) {
                BOOST_THROW_EXCEPTION(
                        keto::wavm_common::ContractExecutionFailedException("Contract execution failed."));
            }
        }
    } catch (...) {
        Runtime::collectGarbage();
        throw;
    }
    Runtime::collectGarbage();
}


void WavmEngineWrapper::executeHttp() {
    // place object in a scope
    try {
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
        Runtime::GCPointer<Compartment> compartment = Runtime::cloneCompartment(
                this->wavmEngineManager.getCompartment());
        Runtime::GCPointer<Context> context = Runtime::createContext(compartment);


        LinkResult linkResult = linkModule(module, *this->wavmEngineManager.getResolver());
        if (!linkResult.success) {
            std::stringstream ss;
            ss << "Failed to link module:";
            for (auto &missingImport : linkResult.missingImports) {
                ss << "Missing import: module=\"" << missingImport.moduleName
                   << "\" export=\"" << missingImport.exportName
                   << "\" type=\"" << asString(missingImport.type) << "\"" << std::endl;
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::LinkingFailedException(ss.str()));
        }

        Runtime::GCPointer<ModuleInstance> moduleInstance =
                instantiateModule(compartment, module, std::move(linkResult.resolvedImports));
        if (!moduleInstance) { BOOST_THROW_EXCEPTION(keto::wavm_common::LinkingFailedException()); }

        // Call the module start function, if it has one.
        Runtime::GCPointer<FunctionInstance> startFunction = getStartFunction(moduleInstance);
        if (startFunction) {
            invokeFunctionChecked(context, startFunction, {});
        }

        keto::Emscripten::initializeGlobals(context, module, moduleInstance);

        // Look up the function export to call.
        Runtime::GCPointer<FunctionInstance> functionInstance;
        functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "request"));

        if (!functionInstance) {
            BOOST_THROW_EXCEPTION(keto::wavm_common::MissingEntryPointException());
        }
        //const FunctionType *functionType = getFunctionType(functionInstance);

        // Set up the arguments for the invoke.
        std::vector<Value> invokeArgs;
        Result functionResult;
        //Timing::Timer executionTimer;
        Runtime::catchRuntimeExceptions(
                [&] {
                    // invoke the function
                    functionResult = invokeFunctionChecked(context, functionInstance, invokeArgs);
                },
                [&](Runtime::Exception &&exception) {
                    std::stringstream ss;
                    ss << "Failed to handle the exception : " << describeException(exception).c_str();

                    KETO_LOG_DEBUG << ss.str();
                    BOOST_THROW_EXCEPTION(keto::wavm_common::ContactExecutionFailedException(ss.str()));
                });

        // validate the result
        bool result = false;
        switch (functionResult.type) {
            case IR::ResultType::i32:
                result = (functionResult.i32);
                break;
            case IR::ResultType::i64:
                result = (functionResult.i64);
                break;
            case IR::ResultType::f32:
                result = (functionResult.f32);
                break;
            case IR::ResultType::f64:
                result = (functionResult.f64);
                break;
            default:
                BOOST_THROW_EXCEPTION(keto::wavm_common::ContractUnsupportedResultException(
                                              "Contract returned an unsupported result."));
        }

    } catch (...) {
        Runtime::collectGarbage();
        throw;
    }
    Runtime::collectGarbage();
}
}
}
