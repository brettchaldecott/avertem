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

//#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/Timing.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/Timing.h"
#include "WAVM/IR/Module.h"
#include "WAVM/IR/Operators.h"
#include "WAVM/IR/Validate.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/Runtime/Runtime.h"
#include "WAVM/Platform/File.h"
#include "WAVM/ThreadTest/ThreadTest.h"
#include "WAVM/WASM/WASM.h"
#include "WAVM/WASI/WASI.h"
#include "WAVM/IR/Types.h"
#include "WAVM/WASTParse/WASTParse.h"

#include "keto/wavm_common/Emscripten.hpp"


#include "keto/wavm_common/WavmEngineWrapper.hpp"
#include "keto/wavm_common/Exception.hpp"
#include "keto/wavm_common/WavmSession.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"
#include "keto/wavm_common/WavmEngineManager.hpp"
#include "keto/wavm_common/WavmSessionTransaction.hpp"


using namespace WAVM;

namespace keto {
namespace wavm_common {


struct RootResolver : WAVM::Runtime::Resolver {
    RootResolver(WAVM::Runtime::Resolver &inInnerResolver, WAVM::Runtime::Compartment *inCompartment)
            : innerResolver(inInnerResolver), compartment(inCompartment) {
    }

    virtual bool resolve(const std::string &moduleName,
                         const std::string &exportName,
                         WAVM::IR::ExternType type,
                         WAVM::Runtime::Object *&outObject) override {
        if (innerResolver.resolve(moduleName, exportName, type, outObject)) { return true; }
        else {
            //KETO_LOG_ERROR << "Resolved import " << moduleName << "." << exportName << " was expecting " << asString(type);
            return generateStub(
                    moduleName, exportName, type, outObject, compartment, WAVM::Runtime::StubFunctionBehavior::trap);
        }
    }

private:
    WAVM::Runtime::Resolver &innerResolver;
    WAVM::Runtime::GCPointer <WAVM::Runtime::Compartment> compartment;

};

bool loadBinaryModule(const std::string& wasmBytes, const WAVM::IR::FeatureSpec& featureSpec, WAVM::Runtime::ModuleRef& outModule)
{
    // Load the module from a binary WebAssembly file.
    try
    {
        WAVM::WASM::LoadError loadError;
        if (!WAVM::Runtime::loadBinaryModule((const U8*)wasmBytes.data(),wasmBytes.size(),outModule,featureSpec,&loadError)) {
            KETO_LOG_ERROR <<  "[WavmEngineWrapper][loadBinaryModule]Error deserializing WebAssembly binary file:";
            KETO_LOG_ERROR <<  loadError.message;
            return false;
        }
    }
    catch(WAVM::Serialization::FatalSerializationException exception)
    {
        KETO_LOG_ERROR <<  "[WavmEngineWrapper][loadBinaryModule]Error deserializing WebAssembly binary file:";
        KETO_LOG_ERROR <<  exception.message;
        return false;
    }
    catch(WAVM::IR::ValidationException exception)
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
    
bool loadTextModule(const std::string& wastString, const WAVM::IR::FeatureSpec& featureSpec, WAVM::Runtime::ModuleRef& outModule)
{
    WAVM::IR::Module irModule(featureSpec);
    std::vector<WAVM::WAST::Error> parseErrors;
    if (!wastString.size()) {
        KETO_LOG_ERROR << "No contract has been supplied and cannot be parsed";
        return false;
    }
    if (!WAVM::WAST::parseModule(wastString.c_str(),wastString.size(),irModule,parseErrors)) {
        KETO_LOG_ERROR <<  "[WavmEngineWrapper][loadBinaryModule]Error deserializing WebAssembly binary file:";
        for (WAVM::WAST::Error error : parseErrors) {
            KETO_LOG_ERROR << error.message;
        }
        return false;
    }
    outModule = WAVM::Runtime::compileModule(irModule);
    return true;
}


std::string WavmEngineWrapper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

WavmEngineWrapper::WavmEngineWrapper(const std::string& wast, const std::string& contract)
    : wast(wast), contract(contract) {
    this->compartment = WAVM::Runtime::createCompartment();
}


WavmEngineWrapper::~WavmEngineWrapper() {
    emscriptenProcess.reset();
    if (!WAVM::Runtime::tryCollectCompartment(std::move(compartment))) {
        //KETO_LOG_ERROR << "Failed to clean out the compartment";
    }
}


void WavmEngineWrapper::execute() {
    WAVM::Runtime::catchRuntimeExceptions([this]() { internalExecute(); },
                                          [](WAVM::Runtime::Exception* exception) {
                                              // Treat any unhandled exception as a fatal error.
                                              //WAVM::Errors::fatalf("Runtime exception: %s",
                                              //               describeException(exception).c_str());
                                              std::stringstream ss;
                                              ss << "\"Runtime exception: " << describeException(exception).c_str();
                                              BOOST_THROW_EXCEPTION(
                                                      keto::wavm_common::ContractExecutionFailedException(ss.str()));
                                          });
}


void WavmEngineWrapper::internalExecute() {
    // place object in a scope
    WAVM::IR::FeatureSpec featureSpec{WAVM::IR::FeatureLevel::wavm};
    WAVM::Runtime::ModuleRef moduleRef;

    if (!loadBinaryModule(wast,featureSpec,moduleRef)) {
        if(!loadTextModule(wast,featureSpec,moduleRef)) {
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidContractException());
        }
    }

    std::vector<std::string> args;
    args.insert(args.begin(), "/proc/1/exe");

    const WAVM::IR::Module& irModule = WAVM::Runtime::getModuleIR(moduleRef);
    emscriptenProcess
            = keto::Emscripten::createProcess(compartment,
                                              std::move(args),
                                              {},
                                              WAVM::Platform::getStdFD(WAVM::Platform::StdDevice::in),
                                              WAVM::Platform::getStdFD(WAVM::Platform::StdDevice::out),
                                              WAVM::Platform::getStdFD(WAVM::Platform::StdDevice::err));
    if(!emscriptenProcess) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::EmscriptInstanciateFailed());
    }

    RootResolver rootResolver(
            keto::Emscripten::getInstanceResolver(*emscriptenProcess), compartment);
    WAVM::Runtime::LinkResult linkResult = WAVM::Runtime::linkModule(irModule, rootResolver);

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

    WAVM::Runtime::Instance* moduleInstance = WAVM::Runtime::instantiateModule(
            compartment, moduleRef, std::move(linkResult.resolvedImports),contract.c_str());
    if(!moduleInstance) {
        std::stringstream ss;
        ss << "Failed to link module: " << contract;
        BOOST_THROW_EXCEPTION(keto::wavm_common::FailedToInstanciateModule(ss.str()));
    }

    // Call the module start function, if it has one.
    WAVM::Runtime::Function* startFunction = WAVM::Runtime::getStartFunction(moduleInstance);
    WAVM::Runtime::Context* context = WAVM::Runtime::createContext(compartment);
    if (startFunction) {
        WAVM::Runtime::invokeFunction(context, startFunction);
    }

    // Initialize the Emscripten instance.
    if (!keto::Emscripten::initializeProcess(*emscriptenProcess, context, irModule, moduleInstance)) {
        std::stringstream ss;
        ss << "Failed to instanciate the initialze the process for contract : " << contract;
        BOOST_THROW_EXCEPTION(keto::wavm_common::FailedToInstanciateModule(ss.str()));
    }

    // Look up the function export to call.
    WAVM::Runtime::Function* functionInstance;
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
    WAVM::IR::FunctionType invokeSig(WAVM::Runtime::getFunctionType(functionInstance));

    std::vector<WAVM::IR::UntaggedValue> untaggedInvokeArgs;
    std::vector<WAVM::IR::UntaggedValue> untaggedInvokeResults;
    untaggedInvokeResults.resize(invokeSig.results().size());
    // Invoke the function.
    WAVM::Timing::Timer executionTimer;
    WAVM::Runtime::invokeFunction(
            context, functionInstance, invokeSig,untaggedInvokeArgs.data(),untaggedInvokeResults.data());
    WAVM::Timing::logTimer("Invoked function", executionTimer);

    if (untaggedInvokeResults.size() != 1) {
        BOOST_THROW_EXCEPTION(
                keto::wavm_common::ContractUnsupportedResultException("Unsupported contract result format."));
    } else if (untaggedInvokeResults.size() == 1) {
        bool result = false;
        switch (invokeSig.results()[0]) {
            case WAVM::IR::ValueType::i32:
                result = untaggedInvokeResults[0].i32;
                break;
            case WAVM::IR::ValueType::i64:
                result = (untaggedInvokeResults[0].i64);
                break;
            case WAVM::IR::ValueType::f32:
                result = (untaggedInvokeResults[0].f32);
                break;
            case WAVM::IR::ValueType::f64:
                result = (untaggedInvokeResults[0].f64);
                break;
            default:
                BOOST_THROW_EXCEPTION(keto::wavm_common::ContractUnsupportedResultException(
                                              "Contract returned an unsupported result."));
        }
        if (!result) {
            BOOST_THROW_EXCEPTION(
                    keto::wavm_common::ContractExecutionFailedException("Contract execution failed."));
        }
    } else {
        BOOST_THROW_EXCEPTION(
                keto::wavm_common::ContractExecutionFailedException("Contract execution failed."));
    }

}

void WavmEngineWrapper::executeHttp() {
    WAVM::Runtime::catchRuntimeExceptions([this]() { internalExecuteHttp(); },
                                    [](WAVM::Runtime::Exception* exception) {
                                        // Treat any unhandled exception as a fatal error.
                                        //WAVM::Errors::fatalf("Runtime exception: %s",
                                        //               describeException(exception).c_str());
                                        std::stringstream ss;
                                        ss << "\"Runtime exception: " << std::endl << describeException(exception).c_str() << std::endl;
                                        BOOST_THROW_EXCEPTION(
                                                keto::wavm_common::ContractExecutionFailedException(ss.str()));
                                    });
}

void WavmEngineWrapper::internalExecuteHttp() {
    // place object in a scope
    WAVM::IR::FeatureSpec featureSpec{WAVM::IR::FeatureLevel::wavm};
    WAVM::Runtime::ModuleRef moduleRef;

    if (!loadBinaryModule(wast, featureSpec, moduleRef)) {
        if (!loadTextModule(wast, featureSpec, moduleRef)) {
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidContractException());
        }
    }

    std::vector<std::string> args;
    args.insert(args.begin(), "/proc/1/exe");

    const WAVM::IR::Module &irModule = WAVM::Runtime::getModuleIR(moduleRef);
    emscriptenProcess
            = keto::Emscripten::createProcess(compartment,
                                              std::move(args),
                                              {},
                                              WAVM::Platform::getStdFD(WAVM::Platform::StdDevice::in),
                                              WAVM::Platform::getStdFD(WAVM::Platform::StdDevice::out),
                                              WAVM::Platform::getStdFD(WAVM::Platform::StdDevice::err));

    if (!emscriptenProcess) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::EmscriptInstanciateFailed());
    }

    RootResolver rootResolver(
            keto::Emscripten::getInstanceResolver(*emscriptenProcess), compartment);
    WAVM::Runtime::LinkResult linkResult = WAVM::Runtime::linkModule(irModule, rootResolver);

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

    WAVM::Runtime::Instance *moduleInstance = WAVM::Runtime::instantiateModule(
            compartment, moduleRef, std::move(linkResult.resolvedImports), contract.c_str());
    if (!moduleInstance) {
        std::stringstream ss;
        ss << "Failed to link module: " << contract;
        BOOST_THROW_EXCEPTION(keto::wavm_common::FailedToInstanciateModule(ss.str()));
    }

    // Call the module start function, if it has one.
    WAVM::Runtime::Function *startFunction = WAVM::Runtime::getStartFunction(moduleInstance);
    WAVM::Runtime::Context *context = WAVM::Runtime::createContext(compartment);
    if (startFunction) {
        WAVM::Runtime::invokeFunction(context, startFunction);
    }

    // Initialize the Emscripten instance.
    if (!keto::Emscripten::initializeProcess(*emscriptenProcess, context, irModule, moduleInstance)) {
        std::stringstream ss;
        ss << "Failed to instanciate the initialze the process for contract : " << contract;
        BOOST_THROW_EXCEPTION(keto::wavm_common::FailedToInstanciateModule(ss.str()));
    }

    // Look up the function export to call.
    WAVM::Runtime::Function *functionInstance;
    functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "request"));

    if (!functionInstance) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::MissingEntryPointException());
    }

    // setup the invoke sig
    WAVM::IR::FunctionType invokeSig(WAVM::Runtime::getFunctionType(functionInstance));

    std::vector<WAVM::IR::UntaggedValue> untaggedInvokeArgs;
    std::vector<WAVM::IR::UntaggedValue> untaggedInvokeResults;
    untaggedInvokeResults.resize(invokeSig.results().size());
    // Invoke the function.
    WAVM::Timing::Timer executionTimer;
    WAVM::Runtime::invokeFunction(
            context, functionInstance, invokeSig, untaggedInvokeArgs.data(), untaggedInvokeResults.data());
    WAVM::Timing::logTimer("Invoked function", executionTimer);

    if (untaggedInvokeResults.size() != 1) {
        BOOST_THROW_EXCEPTION(
                keto::wavm_common::ContractUnsupportedResultException("Unsupported contract result format."));
    } else if (untaggedInvokeResults.size() == 1) {
        bool result = false;
        switch (invokeSig.results()[0]) {
            case WAVM::IR::ValueType::i32:
                result = untaggedInvokeResults[0].i32;
                break;
            case WAVM::IR::ValueType::i64:
                result = (untaggedInvokeResults[0].i64);
                break;
            case WAVM::IR::ValueType::f32:
                result = (untaggedInvokeResults[0].f32);
                break;
            case WAVM::IR::ValueType::f64:
                result = (untaggedInvokeResults[0].f64);
                break;
            default:
                BOOST_THROW_EXCEPTION(keto::wavm_common::ContractUnsupportedResultException(
                                              "Contract returned an unsupported result."));
        }

        if (!result) {
            BOOST_THROW_EXCEPTION(
                    keto::wavm_common::ContractExecutionFailedException("Contract execution failed."));
        }
    } else {
        BOOST_THROW_EXCEPTION(
                keto::wavm_common::ContractExecutionFailedException("Contract execution failed."));
    }
}
}
}
