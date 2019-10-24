#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Logging/Logging.h"
#include "WAVM/IR/IR.h"
#include "WAVM/IR/Types.h"
#include "WAVM/IR/Module.h"
#include "WAVM/Runtime/Runtime.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/VFS/VFS.h"
#include "WAVM/Logging/Logging.h"
#include "WAVM/Platform/Thread.h"

#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <vector>

#include "keto/wavm_common/Emscripten.hpp"
#include "keto/wavm_common/WavmUtils.hpp"
#include "keto/wavm_common/WavmSession.hpp"
#include "keto/wavm_common/WavmSessionTransaction.hpp"
#include "keto/wavm_common/WavmSessionHttp.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"
#include "keto/wavm_common/Exception.hpp"
#include "keto/wavm_common/EmscriptenPrivate.hpp"


#ifndef _WIN32
#include <sys/uio.h>
#include <keto/server_common/ServerInfo.hpp>
#include <keto/wavm_common/Constants.hpp>

#endif

using namespace WAVM::IR;
using namespace WAVM::Runtime;

WAVM::Intrinsics::Module* getIntrinsicModule_env();
WAVM::Intrinsics::Module* getIntrinsicModule_envThreads();
WAVM::Intrinsics::Module* getIntrinsicModule_asm2wasm();
WAVM::Intrinsics::Module* getIntrinsicModule_global();
struct MutableGlobals;

namespace WAVM {
    namespace Emscripten {
    }
}


namespace keto {
    namespace Emscripten
    {

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        //
        // Memory management
        //
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        static constexpr U64 minStaticMemoryPages = 97;

        static constexpr WAVM::Emscripten::emabi::Address mainThreadStackAddress = 64 * WAVM::IR::numBytesPerPage;
        static constexpr WAVM::Emscripten::emabi::Address mainThreadNumStackBytes = 32 * WAVM::IR::numBytesPerPage;

        struct Instance : WAVM::Runtime::Resolver
        {
            WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> moduleInstance;

            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> env;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> keto;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> asm2wasm;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> global;

            WAVM::Runtime::GCPointer<WAVM::Runtime::Memory> memory;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Table> table;

            WAVM::IntrusiveSharedPtr<Thread> mainThread;

            // A global list of running threads created by WebAssembly code.
            WAVM::Platform::Mutex threadsMutex;
            WAVM::IndexMap<WAVM::Emscripten::emabi::pthread_t, WAVM::IntrusiveSharedPtr<Thread>> threads{1, UINT32_MAX};

            std::atomic<WAVM::Emscripten::emabi::pthread_key_t> pthreadSpecificNextKey{0};

            WAVM::Emscripten::emabi::Address errnoAddress{0};

            std::atomic<WAVM::Emscripten::emabi::Address> currentLocale;

            WAVM::VFS::VFD* stdIn{nullptr};
            WAVM::VFS::VFD* stdOut{nullptr};
            WAVM::VFS::VFD* stdErr{nullptr};

            ~Instance();

            bool resolve(const std::string& moduleName,
                         const std::string& exportName,
                         WAVM::IR::ExternType type,
                         WAVM::Runtime::Object*& outObject) override;
        };


        struct MutableGlobals
        {
            enum
            {
                address = 63 * WAVM::IR::numBytesPerPage
            };

            WAVM::Emscripten::emabi::Address DYNAMICTOP_PTR;
            F64 tempDoublePtr;
            WAVM::Emscripten::emabi::FD _stderr;
            WAVM::Emscripten::emabi::FD _stdin;
            WAVM::Emscripten::emabi::FD _stdout;
        };

        bool resizeHeap(Instance* instance, U32 desiredNumBytes)
        {
            const Uptr desiredNumPages
                    = (Uptr(desiredNumBytes) + WAVM::IR::numBytesPerPage - 1) / WAVM::IR::numBytesPerPage;
            const Uptr currentNumPages = WAVM::Runtime::getMemoryNumPages(instance->memory);
            if(desiredNumPages > currentNumPages)
            {
                if(!WAVM::Runtime::growMemory(instance->memory, desiredNumPages - currentNumPages))
                { return false; }

                return true;
            }
            else if(desiredNumPages < currentNumPages)
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        WAVM::Emscripten::emabi::Address dynamicAlloc(Instance* instance, WAVM::Emscripten::emabi::Size numBytes)
        {
            MutableGlobals& mutableGlobals
                    = memoryRef<MutableGlobals>(instance->memory, MutableGlobals::address);

            const WAVM::Emscripten::emabi::Address allocationAddress = mutableGlobals.DYNAMICTOP_PTR;
            const WAVM::Emscripten::emabi::Address endAddress = (allocationAddress + numBytes + 15) & -16;

            mutableGlobals.DYNAMICTOP_PTR = endAddress;

            if(endAddress > getMemoryNumPages(instance->memory) * WAVM::IR::numBytesPerPage)
            {
                Uptr memoryMaxPages = getMemoryType(instance->memory).size.max;
                if(memoryMaxPages == UINT64_MAX) { memoryMaxPages = WAVM::IR::maxMemoryPages; }

                if(endAddress > memoryMaxPages * WAVM::IR::numBytesPerPage || !resizeHeap(instance, endAddress))
                { throwException(ExceptionTypes::outOfMemory); }
            }

            return allocationAddress;
        }

        inline Emscripten::Instance* getEmscriptenInstance(
                WAVM::Runtime::ContextRuntimeData* contextRuntimeData)
        {
            auto instance = (Emscripten::Instance*)getUserData(
                    getCompartmentFromContextRuntimeData(contextRuntimeData));
            WAVM_ASSERT(instance);
            WAVM_ASSERT(instance->memory);
            return instance;
        }

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        //
        // The Thread objects
        //
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        struct Thread
        {
            Emscripten::Instance* instance;
            WAVM::Emscripten::emabi::pthread_t id = 0;
            std::atomic<Uptr> numRefs{0};

            WAVM::Platform::Thread* platformThread = nullptr;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Context> context;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> threadFunc;

            WAVM::Emscripten::emabi::Address stackAddress;
            WAVM::Emscripten::emabi::Address numStackBytes;

            std::atomic<U32> exitCode{0};

            I32 argument;

            WAVM::HashMap<WAVM::Emscripten::emabi::pthread_key_t, WAVM::Emscripten::emabi::Address> pthreadSpecific;

            Thread(Emscripten::Instance* inInstance,
                   WAVM::Runtime::Context* inContext,
                   WAVM::Runtime::Function* inEntryFunction,
                   I32 inArgument)
                    : instance(inInstance)
                    , context(inContext)
                    , threadFunc(inEntryFunction)
                    , argument(inArgument)
            {
            }

            void addRef(Uptr delta = 1) { numRefs += delta; }
            void removeRef()
            {
                if(--numRefs == 0) { delete this; }
            }
        };

        void initThreadLocals(Thread* thread)
        {
            // Call the establishStackSpace function exported by the module to set the thread's stack
            // address and size.
            Function* establishStackSpace = asFunctionNullable(
                    getInstanceExport(thread->instance->moduleInstance, "establishStackSpace"));
            if(establishStackSpace
               && getFunctionType(establishStackSpace)
                  == FunctionType(TypeTuple{}, TypeTuple{ValueType::i32, ValueType::i32}))
            {
                WAVM::IR::UntaggedValue args[2]{thread->stackAddress, thread->numStackBytes};
                invokeFunction(thread->context,
                               establishStackSpace,
                               FunctionType({}, {ValueType::i32, ValueType::i32}),
                               args);
            }
        }

        void joinAllThreads(Instance* instance)
        {
            while(true)
            {
                WAVM::Platform::Mutex::Lock threadsLock(instance->threadsMutex);

                if(!instance->threads.size()) { break; }
                auto it = instance->threads.begin();
                WAVM_ASSERT(it != instance->threads.end());

                WAVM::Emscripten::emabi::pthread_t threadId = it.getIndex();
                WAVM::IntrusiveSharedPtr<Thread> thread = std::move(*it);
                instance->threads.removeOrFail(threadId);

                threadsLock.unlock();

                WAVM::Platform::joinThread(thread->platformThread);
                thread->platformThread = nullptr;
            };
        }


        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        //
        // The Emscripten Instance
        //
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        Instance::~Instance()
        {
            // Instead of allowing an Instance to live on until all its threads exit, wait for all threads
            // to exit before destroying the Instance.
            joinAllThreads(this);
        }

        bool Instance::resolve(const std::string& moduleName,
                                           const std::string& exportName,
                                           WAVM::IR::ExternType type,
                                           WAVM::Runtime::Object*& outObject)
        {
            WAVM::Runtime::Instance* intrinsicInstance
                    = moduleName == "env"
                      ? env
                      : moduleName == "asm2wasm" ? asm2wasm : moduleName == "global" ? global : nullptr;
            if(intrinsicInstance)
            {
                outObject = getInstanceExport(intrinsicInstance, exportName);
                if(outObject)
                {
                    if(isA(outObject, type)) { return true; }
                    else
                    {
                        WAVM::Log::printf(WAVM::Log::debug,
                                          "Resolved import %s.%s to a %s, but was expecting %s\n",
                                          moduleName.c_str(),
                                          exportName.c_str(),
                                          asString(getExternType(outObject)).c_str(),
                                          asString(type).c_str());
                    }
                }
            }

            return false;
        }

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        //
        // Keto Instrict Module
        //
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        WAVM_DEFINE_INTRINSIC_MODULE(keto)


        keto::wavm_common::WavmSessionTransactionPtr castToTransactionSession(keto::wavm_common::WavmSessionPtr wavmSessionPtr) {
            if (wavmSessionPtr->getSessionType() == keto::wavm_common::Constants::SESSION_TYPES::TRANSACTION)  {
                return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransaction>(wavmSessionPtr);
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidWavmSessionTypeException());
        }

        keto::wavm_common::WavmSessionHttpPtr castToTHttpSession(keto::wavm_common::WavmSessionPtr wavmSessionPtr) {
            if (wavmSessionPtr->getSessionType() == keto::wavm_common::Constants::SESSION_TYPES::HTTP)  {
                return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionHttp>(wavmSessionPtr);
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidWavmSessionTypeException());
        }

        I32 createCstringBuf(Instance* instance, const std::string& returnString ) {
            int length = returnString.size() + 1;

            I32 base = dynamicAlloc(instance,length);
            memset(WAVM::Runtime::memoryArrayPtr<U8>(instance->memory,base,length),0,length);
            memcpy(WAVM::Runtime::memoryArrayPtr<U8>(instance->memory,base,length),returnString.data(),
                   returnString.size());
            return base;
        }

        // type script method mappings
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__console",void,keto_console,I32 msg)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            return;
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__log",void,keto_log,I32 level,I32 msg)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            keto::wavm_common::WavmUtils::log(U32(level),msgString);
            return;
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getFeeAccount",I32,keto_getFeeAccount)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountHash = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getFeeAccount();
            return createCstringBuf(instance,accountHash);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getAccount",I32,keto_getAccount)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getAccount();
            return createCstringBuf(instance,account);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getTransaction",I32,keto_getTransaction)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string transaction = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransaction();
            return createCstringBuf(instance,transaction);
        }

        // rdf methods
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_executeQuery",I64,keto_rdf_executeQuery,I32 type, I32 query)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string queryStr = keto::wavm_common::WavmUtils::readCString(instance->memory,query);
            std::string typeStr = keto::wavm_common::WavmUtils::readCString(instance->memory,type);

            return (I64)(long)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->executeQuery(typeStr,queryStr);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryHeaderCount",I64,keto_rdf_getQueryHeaderCount,I64 id)
        {
            return (I64)(long)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryHeaderCount(id);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryHeader",I32,keto_rdf_getQueryHeader,I64 id, I64 index)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string header = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryHeader(id,index);
            return createCstringBuf(instance,header);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryString",I32,keto_rdf_getQueryStringValue,I64 id, I64 index, I64 headerNumber)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string value = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryStringValue(id,index,headerNumber);
            return createCstringBuf(instance,value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryStringByKey",I32,keto_rdf_getQueryStringByKey,I64 id, I64 index, I32 name)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            std::string value = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryStringValue(id,index,nameString);
            return createCstringBuf(instance,value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryLong",I64,keto_rdf_getQueryLong,I64 id, I64 index, I64 headerNumber)
        {
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryLongValue(id,index,headerNumber);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryLongByKey",I64,keto_rdf_getQueryLongByKey,I64 id, I64 index, I32 name)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryLongValue(id,index,nameString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryFloat",I32,keto_rdf_getQueryFloat,I64 id, I64 index, I64 headerNumber)
        {
            return (I32)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryFloatValue(id,index,headerNumber);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryFloatByKey",I32,keto_rdf_getQueryFloatByKey,I64 id, I64 index, I32 name)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            return (I32)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryFloatValue(id,index,nameString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getRowCount",I64,keto_rdf_getRowCount,I64 id)
        {
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getRowCount(id);
        }


        // transaction methods
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getTransactionValue",I64,keto_getTransactionValue)
        {
        return (I64)(long)castToTransactionSession(
        keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransactionValue();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getTotalFeeValue",I64,keto_getTotalFeeValue, I64 minimum)
        {
        return (I64)(long)castToTransactionSession(
        keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTotalTransactionFee(minimum);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getFeeValue",I64,keto_getFeeValue, I64 minimum)
        {
        return (I64)(long)castToTransactionSession(
        keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransactionFee(minimum);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestModelTransactionValue",I64,keto_getRequestModelTransactionValue,I32 accountModel,I32 transactionValueModel)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,transactionValueModel);

            return (I64)(long)castToTransactionSession(
            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            getRequestModelTransactionValue(accountModelString,transactionValueModelString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__createDebitEntry",void,keto_createDebitEntry,I32 accountId, I32 name, I32 description, I32 accountModel,I32 transactionValueModel,I64 value)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountIdString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountId);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            std::string descriptionString = keto::wavm_common::WavmUtils::readCString(instance->memory,description);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,transactionValueModel);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->createDebitEntry(
                    accountIdString, nameString, descriptionString, accountModelString,transactionValueModelString,(U64)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__createCreditEntry",void,keto_createCreditEntry, I32 accountId, I32 name, I32 description, I32 accountModel,I32 transactionValueModel,I64 value)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountIdString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountId);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            std::string descriptionString = keto::wavm_common::WavmUtils::readCString(instance->memory,description);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,transactionValueModel);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->createCreditEntry(
                    accountIdString, nameString, descriptionString, accountModelString,transactionValueModelString,(U64)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestStringValue",I32,keto_getRequestStringValue,I32 subject,I32 predicate)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            std::string requestString = castToTransactionSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRequestStringValue(subjectString,predicateString);
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseStringValue",void,keto_setResponseStringValue,I32 subject,I32 predicate,I32 value)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);
            std::string valueString = keto::wavm_common::WavmUtils::readCString(instance->memory,value);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setResponseStringValue(
                    subjectString,predicateString,valueString);
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestLongValue",I64,keto_getRequestLongValue,I32 subject,I32 predicate)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            return (I64) castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRequestLongValue(
                            subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseLongValue",void,keto_setResponseLongValue,I32 subject,I32 predicate, I64 value)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setResponseLongValue(
                            subjectString,predicateString,(U64)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getResponseFloatValue",I32,keto_getRequestFloatValue,I32 subject,I32 predicate)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            return (I32)castToTransactionSession(
            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            getRequestFloatValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseFloatValue",void,keto_setResponseFloatValue,I32 subject,I32 predicate,I32 value)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->
            getWavmSession())->setResponseFloatValue(subjectString,predicateString,(U32)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestBooleanValue",I32,keto_getRequestBooleanValue,I32 subject,I32 predicate)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            return (bool) castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->
            getWavmSession())->getRequestBooleanValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseBooleanValue",void,keto_setResponseBooleanValue,I32 subject,I32 predicate,I32 value)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            castToTransactionSession(
            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            setResponseBooleanValue(subjectString,predicateString,(bool)value);
        }

        //
        // http session wrapping
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getNumberOfRoles",I64,keto_http_getNumberOfRoles)
        {
            return (I64)(long)castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getNumberOfRoles();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getRole",I32,keto_http_getRole,I64 index)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRole(index);
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getTargetUri",I32,keto_http_getTargetUri)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTargetUri();
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getQuery",I32,keto_http_getQuery)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getQuery();
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getMethod",I32,keto_http_getMethod)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getMethod();
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getBody",I32,keto_http_getBody)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getBody();
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getNumberOfParameters",I64,keto_http_getNumberOfParameters)
        {
            return (I64)(long)castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getNumberOfParameters();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getParameterKey",I32,keto_http_getParameterKey,I64 index)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getParameterKey(index);
            return createCstringBuf(instance,requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getParameter",I32,keto_http_getParameter,I32 key)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string keyString = keto::wavm_common::WavmUtils::readCString(instance->memory,key);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getParameter(keyString);
            return createCstringBuf(instance,requestString);
        }

        // response methods
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_setStatus",void,keto_http_setStatus,I64 statusCode)
        {
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setStatus(statusCode);
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_setContentType",void,keto_http_setContentType,I32 contentType)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string contentTypeString = keto::wavm_common::WavmUtils::readCString(instance->memory,contentType);
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setContentType(contentTypeString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_setBody",void,keto_http_setBody,I32 body)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string bodyString = keto::wavm_common::WavmUtils::readCString(instance->memory,body);
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setBody(bodyString);
        }

        // C/CPP method mappings
        WAVM_DEFINE_INTRINSIC_FUNCTION(env,"console",void,c_console,I32 msg)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            return;
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(env,"log",void,c_log,I64 level,I32 msg)
        {
            Emscripten::Instance* instance = getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            keto::wavm_common::WavmUtils::log(U32(level),msgString);
            return;
        }

        // transaction builder api

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        //
        // The Emscripten
        //
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        std::shared_ptr<Emscripten::Instance> instantiate(Compartment* compartment,
                                                                      const WAVM::IR::Module& module,
                                                                      WAVM::VFS::VFD* stdIn,
                                                                      WAVM::VFS::VFD* stdOut,
                                                                      WAVM::VFS::VFD* stdErr)
        {
            MemoryType memoryType(false, SizeConstraints{0, 0});
            if(module.memories.imports.size() && module.memories.imports[0].moduleName == "env"
               && module.memories.imports[0].exportName == "memory")
            {
                memoryType = module.memories.imports[0].type;
                if(memoryType.size.max >= minStaticMemoryPages)
                {
                    if(memoryType.size.min <= minStaticMemoryPages)
                    {
                        // Enlarge the initial memory to make space for the stack and mutable globals.
                        memoryType.size.min = minStaticMemoryPages;
                    }
                }
                else
                {
                    WAVM::Log::printf(WAVM::Log::error, "module's memory is too small for Emscripten emulation\n");
                    return nullptr;
                }
            }
            else
            {
                WAVM::Log::printf(WAVM::Log::error, "module does not import Emscripten's env.memory\n");
                return nullptr;
            }

            TableType tableType(ReferenceType::funcref, false, SizeConstraints{0, 0});
            if(module.tables.imports.size() && module.tables.imports[0].moduleName == "env"
               && module.tables.imports[0].exportName == "table")
            { tableType = module.tables.imports[0].type; }

            Memory* memory = WAVM::Runtime::createMemory(compartment, memoryType, "env.memory");
            Table* table = WAVM::Runtime::createTable(compartment, tableType, nullptr, "env.table");

            WAVM::HashMap<std::string, WAVM::Runtime::Object*> extraEnvExports = {
                    {"memory", WAVM::Runtime::asObject(memory)},
                    {"table", WAVM::Runtime::asObject(table)},
            };

            std::shared_ptr<Instance> instance = std::make_shared<Instance>();
            instance->env = WAVM::Intrinsics::instantiateModule(
                    compartment,
                    {WAVM_INTRINSIC_MODULE_REF(env), WAVM_INTRINSIC_MODULE_REF(envThreads)},
                    "env",
                    extraEnvExports);
            instance->keto = WAVM::Intrinsics::instantiateModule(
                    compartment,
                    {WAVM_INTRINSIC_MODULE_REF(keto)},
                    "keto",
                    extraEnvExports);
            instance->asm2wasm = WAVM::Intrinsics::instantiateModule(
                    compartment, {WAVM_INTRINSIC_MODULE_REF(asm2wasm)}, "asm2wasm");
            instance->global
                    = WAVM::Intrinsics::instantiateModule(compartment, {WAVM_INTRINSIC_MODULE_REF(global)}, "global");

            unwindSignalsAsExceptions([=] {
                MutableGlobals& mutableGlobals = memoryRef<MutableGlobals>(memory, MutableGlobals::address);

                mutableGlobals.DYNAMICTOP_PTR = minStaticMemoryPages * WAVM::IR::numBytesPerPage;
                mutableGlobals._stderr = (U32)WAVM::Emscripten::ioStreamVMHandle::StdErr;
                mutableGlobals._stdin = (U32)WAVM::Emscripten::ioStreamVMHandle::StdIn;
                mutableGlobals._stdout = (U32)WAVM::Emscripten::ioStreamVMHandle::StdOut;
            });

            instance->compartment = compartment;
            instance->memory = memory;
            instance->table = table;

            instance->stdIn = stdIn;
            instance->stdOut = stdOut;
            instance->stdErr = stdErr;

            setUserData(compartment, instance.get());

            return instance;
        }

        void initializeGlobals(const std::shared_ptr<Instance>& instance,
                                           Context* context,
                                           const WAVM::IR::Module& module,
                                            WAVM::Runtime::Instance* moduleInstance)
        {
            instance->moduleInstance = moduleInstance;

            // Create an Emscripten "main thread" and associate it with this context.
            instance->mainThread = new Emscripten::Thread(instance.get(), context, nullptr, 0);
            setUserData(context, instance->mainThread);

            instance->mainThread->stackAddress = mainThreadStackAddress;
            instance->mainThread->numStackBytes = mainThreadNumStackBytes;

            // Initialize the Emscripten "thread local" state.
            initThreadLocals(instance->mainThread);

            // Call the global initializer functions: newer Emscripten uses a single globalCtors function,
            // and older Emscripten uses a __GLOBAL__* function for each translation unit.
            if(Function* globalCtors = asFunctionNullable(getInstanceExport(moduleInstance, "globalCtors")))
            { invokeFunction(context, globalCtors); }

            for(Uptr exportIndex = 0; exportIndex < module.exports.size(); ++exportIndex)
            {
                const Export& functionExport = module.exports[exportIndex];
                if(functionExport.kind == WAVM::IR::ExternKind::function
                   && !strncmp(functionExport.name.c_str(), "__GLOBAL__", 10))
                {
                    Function* function
                            = asFunctionNullable(getInstanceExport(moduleInstance, functionExport.name));
                    if(function) { invokeFunction(context, function); }
                }
            }

            // Store ___errno_location.
            Function* errNoLocation
                    = asFunctionNullable(getInstanceExport(moduleInstance, "___errno_location"));
            if(errNoLocation && getFunctionType(errNoLocation) == FunctionType({ValueType::i32}, {}))
            {
                WAVM::IR::UntaggedValue errNoResult;
                invokeFunction(
                        context, errNoLocation, FunctionType({ValueType::i32}, {}), nullptr, &errNoResult);
                instance->errnoAddress = errNoResult.i32;
            }
        }

        std::vector<WAVM::IR::Value> injectCommandArgs(const std::shared_ptr<Instance>& instance,
                                                             const std::vector<std::string>& argStrings)
        {
            U8* memoryBase = getMemoryBaseAddress(instance->memory);

            U32* argvOffsets
                    = (U32*)(memoryBase
                             + dynamicAlloc(instance.get(), (U32)(sizeof(U32) * (argStrings.size() + 1))));
            for(Uptr argIndex = 0; argIndex < argStrings.size(); ++argIndex)
            {
                auto stringSize = argStrings[argIndex].size() + 1;
                auto stringMemory = memoryBase + dynamicAlloc(instance.get(), (U32)stringSize);
                memcpy(stringMemory, argStrings[argIndex].c_str(), stringSize);
                argvOffsets[argIndex] = (U32)(stringMemory - memoryBase);
            }
            argvOffsets[argStrings.size()] = 0;
            return {(U32)argStrings.size(), (U32)((U8*)argvOffsets - memoryBase)};
        }

        WAVM::Runtime::Resolver& getInstanceResolver(const std::shared_ptr<Instance>& instance)
        {
            return *instance.get();
        }


        I32 catchExit(std::function<I32()>&& thunk)
        {
            try
            {
                return std::move(thunk)();
            }
            catch(WAVM::Emscripten::ExitException const& exitException)
            {
                return I32(exitException.exitCode);
            }
        }


        /*// TODO: This code is currently not very efficient for multi threading
        //using namespace IR;
        //using namespace Runtime;

        DEFINE_INTRINSIC_MODULE(env)
        DEFINE_INTRINSIC_MODULE(asm2wasm)
        DEFINE_INTRINSIC_MODULE(global)
        DEFINE_INTRINSIC_MODULE(keto)


        Runtime::MemoryInstance* emscriptenMemoryInstance = nullptr;

        static U32 coerce32bitAddress(Uptr address)
        {
            if(address >= UINT32_MAX) { throwException(Runtime::Exception::accessViolationType); }
            return (U32)address;
        }

        enum { initialNumPages = 256 };
        enum { initialNumTableElements = 1024 * 1024 };

        struct MutableGlobals
        {
            enum { address = 127 * WAVM::IR::numBytesPerPage };

            F64 tempDoublePtr;
            I32 _stderr;
            I32 _stdin;
            I32 _stdout;
        };

        DEFINE_INTRINSIC_MEMORY(env,emscriptenMemory,memory,WAVM::IR::MemoryType(false,WAVM::IR::SizeConstraints({initialNumPages,UINT64_MAX })));
        DEFINE_INTRINSIC_TABLE(env,table,table,WAVM::IR::TableType(WAVM::IR::TableElementType::anyfunc,false,WAVM::IR::SizeConstraints({initialNumTableElements,UINT64_MAX})));

        DEFINE_INTRINSIC_GLOBAL(env,"STACKTOP",I32,STACKTOP          ,128 * WAVM::IR::numBytesPerPage);
        DEFINE_INTRINSIC_GLOBAL(env,"STACK_MAX",I32,STACK_MAX        ,256 * WAVM::IR::numBytesPerPage);
        DEFINE_INTRINSIC_GLOBAL(env,"tempDoublePtr",I32,tempDoublePtr,127 * WAVM::IR::numBytesPerPage + offsetof(MutableGlobals,tempDoublePtr));
        DEFINE_INTRINSIC_GLOBAL(env,"ABORT",I32,ABORT                ,0);
        DEFINE_INTRINSIC_GLOBAL(env,"cttz_i8",I32,cttz_i8            ,0);
        DEFINE_INTRINSIC_GLOBAL(env,"___dso_handle",I32,___dso_handle,0);
        DEFINE_INTRINSIC_GLOBAL(env,"_stderr",I32,_stderr            ,127 * WAVM::IR::numBytesPerPage + offsetof(MutableGlobals,_stderr));
        DEFINE_INTRINSIC_GLOBAL(env,"_stdin",I32,_stdin              ,127 * WAVM::IR::numBytesPerPage + offsetof(MutableGlobals,_stdin));
        DEFINE_INTRINSIC_GLOBAL(env,"_stdout",I32,_stdout            ,127 * WAVM::IR::numBytesPerPage + offsetof(MutableGlobals,_stdout));

        DEFINE_INTRINSIC_GLOBAL(env,"memoryBase",I32,emscriptenMemoryBase,1024);
        DEFINE_INTRINSIC_GLOBAL(env,"tableBase",I32,emscriptenTableBase,0);

        DEFINE_INTRINSIC_GLOBAL(env,"DYNAMICTOP_PTR",I32,DYNAMICTOP_PTR,0)
        DEFINE_INTRINSIC_GLOBAL(env,"_environ",I32,em_environ,0)
        DEFINE_INTRINSIC_GLOBAL(env,"EMTSTACKTOP",I32,EMTSTACKTOP,0)
        DEFINE_INTRINSIC_GLOBAL(env,"EMT_STACK_MAX",I32,EMT_STACK_MAX,0)
        DEFINE_INTRINSIC_GLOBAL(env,"eb",I32,eb,0)

        Platform::Mutex* sbrkMutex = Platform::createMutex();
        bool hasSbrkBeenCalled = false;
        Uptr sbrkNumPages = 0;
        U32 sbrkMinBytes = 0;
        U32 sbrkNumBytes = 0;

        // this function is only compatible with
        static U32 sbrk(I32 numBytes)
        {
            Platform::Lock sbrkLock(sbrkMutex);
            if(!hasSbrkBeenCalled)
            {
                // Do some first time initialization.
                sbrkNumPages = getMemoryNumPages(emscriptenMemoryInstance);
                sbrkMinBytes = sbrkNumBytes = coerce32bitAddress(sbrkNumPages << WAVM::IR::numBytesPerPageLog2);
                hasSbrkBeenCalled = true;
            }
            else
            {
                // Ensure that nothing else is calling growMemory/shrinkMemory.
                if(getMemoryNumPages(emscriptenMemoryInstance) != sbrkNumPages)
                { throwException(Runtime::Exception::outOfMemoryType); }
            }

            const U32 previousNumBytes = sbrkNumBytes;

            // Round the absolute value of numBytes to an alignment boundary, and ensure it won't allocate too much or too little memory.
            numBytes = (numBytes + 7) & ~7;
            if(numBytes > 0 && previousNumBytes > UINT32_MAX - numBytes) { throwException(Runtime::Exception::accessViolationType); }
            else if(numBytes < 0 && previousNumBytes < sbrkMinBytes - numBytes) { throwException(Runtime::Exception::accessViolationType); }

            // Update the number of bytes allocated, and compute the number of pages needed for it.
            sbrkNumBytes += numBytes;
            const Uptr numDesiredPages = (sbrkNumBytes + WAVM::IR::numBytesPerPage - 1) >> WAVM::IR::numBytesPerPageLog2;

            // Grow or shrink the memory object to the desired number of pages.
            if(numDesiredPages > sbrkNumPages) { growMemory(emscriptenMemoryInstance,numDesiredPages - sbrkNumPages); }
            else if(numDesiredPages < sbrkNumPages) { shrinkMemory(emscriptenMemoryInstance,sbrkNumPages - numDesiredPages); }
            sbrkNumPages = numDesiredPages;

            return previousNumBytes;
        }

        static U32 allocateMemory(Runtime::MemoryInstance* memory, I32 numBytes)
        {
            Platform::Lock sbrkLock(sbrkMutex);

            int _currentPages = getMemoryNumPages(memory);
            int _currentNumBytes = coerce32bitAddress(_currentPages << WAVM::IR::numBytesPerPageLog2);

            const U32 previousNumBytes = _currentNumBytes;

            // Round the absolute value of numBytes to an alignment boundary, and ensure it won't allocate too much or too little memory.
            numBytes = (numBytes + 7) & ~7;

            // Update the number of bytes allocated, and compute the number of pages needed for it.
            _currentNumBytes += numBytes;
            const Uptr numDesiredPages = (_currentNumBytes + WAVM::IR::numBytesPerPage - 1) >> WAVM::IR::numBytesPerPageLog2;

            // Grow or shrink the memory object to the desired number of pages.
            if(numDesiredPages > _currentPages) { growMemory(memory,numDesiredPages - _currentPages); }

            return previousNumBytes;
        }

        DEFINE_INTRINSIC_FUNCTION(env,"_sbrk",I32,_sbrk,I32 numBytes)
        {
            return sbrk(numBytes);
        }

        DEFINE_INTRINSIC_FUNCTION(env,"_time",I32,_time,I32 address)
        {
            time_t t = time(nullptr);
            if(address)
            {
                Runtime::memoryRef<I32>(emscriptenMemoryInstance,address) = (I32)t;
            }
            return (I32)t;
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___errno_location",I32,___errno_location)
        {
            return 0;
        }

        DEFINE_INTRINSIC_FUNCTION(env,"_sysconf",I32,_sysconf,I32 a)
        {
            enum { sysConfPageSize = 30 };
            switch(a)
            {
                case sysConfPageSize: return WAVM::IR::numBytesPerPage;
                default: throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
            }
        }

        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_cond_wait",I32,_pthread_cond_wait,I32 a,I32 b) { return 0; }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_cond_broadcast",I32,_pthread_cond_broadcast,I32 a) { return 0; }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_key_create",I32,_pthread_key_create,I32 a,I32 b) { throwException(Runtime::Exception::calledUnimplementedIntrinsicType); }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_mutex_lock",I32,_pthread_mutex_lock,I32 a) { return 0; }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_mutex_unlock",I32,_pthread_mutex_unlock,I32 a) { return 0; }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_setspecific",I32,_pthread_setspecific,I32 a,I32 b) { throwException(Runtime::Exception::calledUnimplementedIntrinsicType); }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_getspecific",I32,_pthread_getspecific,I32 a) { throwException(Runtime::Exception::calledUnimplementedIntrinsicType); }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_once",I32,_pthread_once,I32 a,I32 b) { throwException(Runtime::Exception::calledUnimplementedIntrinsicType); }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_cleanup_push",void,_pthread_cleanup_push,I32 a,I32 b) { }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_cleanup_pop",void,_pthread_cleanup_pop,I32 a) { }
        DEFINE_INTRINSIC_FUNCTION(env,"_pthread_self",I32,_pthread_self) { return 0; }

        DEFINE_INTRINSIC_FUNCTION(env,"___ctype_b_loc",I32,___ctype_b_loc)
        {
            unsigned short data[384] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,8195,8194,8194,8194,8194,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,24577,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,55304,55304,55304,55304,55304,55304,55304,55304,55304,55304,49156,49156,49156,49156,49156,49156,49156,54536,54536,54536,54536,54536,54536,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,49156,49156,49156,49156,49156,49156,54792,54792,54792,54792,54792,54792,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,49156,49156,49156,49156,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            static U32 vmAddress = 0;
            if(vmAddress == 0)
            {
                vmAddress = coerce32bitAddress(sbrk(sizeof(data)));
                memcpy(Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,vmAddress,sizeof(data)),data,sizeof(data));
            }
            return vmAddress + sizeof(short)*128;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___ctype_toupper_loc",I32,___ctype_toupper_loc)
        {
            I32 data[384] = {128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
            static U32 vmAddress = 0;
            if(vmAddress == 0)
            {
                vmAddress = coerce32bitAddress(sbrk(sizeof(data)));
                memcpy(Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,vmAddress,sizeof(data)),data,sizeof(data));
            }
            return vmAddress + sizeof(I32)*128;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___ctype_tolower_loc",I32,___ctype_tolower_loc)
        {
            I32 data[384] = {128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
            static U32 vmAddress = 0;
            if(vmAddress == 0)
            {
                vmAddress = coerce32bitAddress(sbrk(sizeof(data)));
                memcpy(Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,vmAddress,sizeof(data)),data,sizeof(data));
            }
            return vmAddress + sizeof(I32)*128;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___assert_fail",void,___assert_fail,I32 condition,I32 filename,I32 line,I32 function)
        {
            throwException(Runtime::Exception::calledAbortType);
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___cxa_atexit",I32,___cxa_atexit,I32 a,I32 b,I32 c)
        {
            return 0;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___cxa_guard_acquire",I32,___cxa_guard_acquire,I32 address)
        {
            if(!Runtime::memoryRef<U8>(emscriptenMemoryInstance,address))
            {
                Runtime::memoryRef<U8>(emscriptenMemoryInstance,address) = 1;
                return 1;
            }
            else
            {
                return 0;
            }
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___cxa_guard_release",void,___cxa_guard_release,I32 a)
        {}
        DEFINE_INTRINSIC_FUNCTION(env,"___cxa_throw",void,___cxa_throw,I32 a,I32 b,I32 c)
        {
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___cxa_begin_catch",I32,___cxa_begin_catch,I32 a)
        {
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___cxa_allocate_exception",I32,___cxa_allocate_exception,I32 size)
        {
            return coerce32bitAddress(sbrk(size));
        }
        DEFINE_INTRINSIC_FUNCTION(env,"__ZSt18uncaught_exceptionv",I32,__ZSt18uncaught_exceptionv)
        {
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_abort",void,_abort)
        {
            throwException(Runtime::Exception::calledAbortType);
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_exit",void,_exit,I32 code)
        {
            throwException(Runtime::Exception::calledAbortType);
        }
        DEFINE_INTRINSIC_FUNCTION(env,"abort",void,abort,I32 code)
        {
            Log::printf(Log::Category::error,"env.abort(%i)\n",code);
            throwException(Runtime::Exception::calledAbortType);
        }

        static U32 currentLocale = 0;
        DEFINE_INTRINSIC_FUNCTION(env,"_uselocale",I32,_uselocale,I32 locale)
        {
            auto oldLocale = currentLocale;
            currentLocale = locale;
            return oldLocale;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_newlocale",I32,_newlocale,I32 mask,I32 locale,I32 base)
        {
            if(!base)
            {
                base = coerce32bitAddress(sbrk(4));
            }
            return base;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_freelocale",void,_freelocale,I32 a)
        {}

        DEFINE_INTRINSIC_FUNCTION(env,"_strftime_l",I32,_strftime_l,I32 a,I32 b,I32 c,I32 d,I32 e) { throwException(Runtime::Exception::calledUnimplementedIntrinsicType); }
        DEFINE_INTRINSIC_FUNCTION(env,"_strerror",I32,_strerror,I32 a) { throwException(Runtime::Exception::calledUnimplementedIntrinsicType); }

        DEFINE_INTRINSIC_FUNCTION(env,"_catopen",I32,_catopen,I32 a,I32 b) { return (U32)-1; }
        DEFINE_INTRINSIC_FUNCTION(env,"_catgets",I32,_catgets,I32 catd,I32 set_id,I32 msg_id,I32 s) { return s; }
        DEFINE_INTRINSIC_FUNCTION(env,"_catclose",I32,_catclose,I32 a) { return 0; }

        DEFINE_INTRINSIC_FUNCTION(env,"_emscripten_memcpy_big",I32,_emscripten_memcpy_big,I32 a,I32 b,I32 c)
        {
            memcpy(Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,a,c),Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,b,c),U32(c));
            return a;
        }

        enum class ioStreamVMHandle
        {
            StdErr = 1,
            StdIn = 2,
            StdOut = 3
        };
        FILE* vmFile(U32 vmHandle)
        {
            switch((ioStreamVMHandle)vmHandle)
            {
                case ioStreamVMHandle::StdErr: return stderr;
                case ioStreamVMHandle::StdIn: return stdin;
                case ioStreamVMHandle::StdOut: return stdout;
                    default: return stdout;//std::cerr << "invalid file handle " << vmHandle << std::endl; throw;
            }
        }

        DEFINE_INTRINSIC_FUNCTION(env,"_vfprintf",I32,_vfprintf,I32 file,I32 formatPointer,I32 argList)
        {
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_getc",I32,_getc,I32 file)
        {
            return getc(vmFile(file));
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_ungetc",I32,_ungetc,I32 character,I32 file)
        {
            return ungetc(character,vmFile(file));
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_fread",I32,_fread,I32 pointer,I32 size,I32 count,I32 file)
        {
            return (I32)fread(Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,pointer,U64(size) * U64(count)),U64(size),U64(count),vmFile(file));
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_fwrite",I32,_fwrite,I32 pointer,I32 size,I32 count,I32 file)
        {
            return (I32)fwrite(Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,pointer,U64(size) * U64(count)),U64(size),U64(count),vmFile(file));
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_fputc",I32,_fputc,I32 character,I32 file)
        {
            return fputc(character,vmFile(file));
        }
        DEFINE_INTRINSIC_FUNCTION(env,"_fflush",I32,_fflush,I32 file)
        {
            return fflush(vmFile(file));
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___lock",void,___lock,I32 a)
        {
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___unlock",void,___unlock,I32 a)
        {
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___lockfile",I32,___lockfile,I32 a)
        {
            return 1;
        }
        DEFINE_INTRINSIC_FUNCTION(env,"___unlockfile",void,___unlockfile,I32 a)
        {
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___syscall6",I32,___syscall6,I32 a,I32 b)
        {
            // close
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___syscall54",I32,___syscall54,I32 a,I32 b)
        {
            // ioctl
            return 0;
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___syscall140",I32,___syscall140,I32 a,I32 b)
        {
            // llseek
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }

        DEFINE_INTRINSIC_FUNCTION(env,"___syscall145",I32,___syscall145,I32 file,I32 argsPtr)
        {
            // readv
            throwException(Runtime::Exception::calledUnimplementedIntrinsicType);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(env,"___syscall146",I32,___syscall146,I32 file,I32 argsPtr)
        {
            //Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            // writev
            U32* args = Runtime::memoryArrayPtr<U32>(emscriptenMemoryInstance,argsPtr,3);
            U32 iov = args[1];
            U32 iovcnt = args[2];
#ifdef _WIN32
            U32 count = 0;
            for(U32 i = 0; i < iovcnt; i++)
            {
                U32 base = Runtime::memoryRef<U32>(emscriptenMemoryInstance,iov + i * 8);
                U32 len = Runtime::memoryRef<U32>(emscriptenMemoryInstance,iov + i * 8 + 4);
                U32 size = (U32)fwrite(memoryArrayPtr<U8>(emscriptenMemoryInstance,base,len), 1, len, vmFile(file));
                count += size;
                if (size < len)
                    break;
            }
#else
            struct iovec *native_iovec = new(alloca(sizeof(iovec)*iovcnt)) struct iovec [iovcnt];
            for(U32 i = 0; i < iovcnt; i++)
            {
                U32 base = Runtime::memoryRef<U32>(emscriptenMemoryInstance,iov + i * 8);
                U32 len = Runtime::memoryRef<U32>(emscriptenMemoryInstance,iov + i * 8 + 4);

                native_iovec[i].iov_base = Runtime::memoryArrayPtr<U8>(emscriptenMemoryInstance,base,len);
                native_iovec[i].iov_len = len;
            }
            Iptr count = writev(fileno(vmFile(file)), native_iovec, iovcnt);
#endif
            return count;
        }

        DEFINE_INTRINSIC_FUNCTION(asm2wasm,"f64-to-int",I32,f64_to_int,F64 f) { return (I32)f; }

        static F64 zero = 0.0;

        static F64 makeNaN() { return zero / zero; }
        static F64 makeInf() { return 1.0/zero; }

        DEFINE_INTRINSIC_GLOBAL(global,"NaN",F64,NaN,makeNaN())
        DEFINE_INTRINSIC_GLOBAL(global,"Infinity",F64,Infinity,makeInf())

        DEFINE_INTRINSIC_FUNCTION(asm2wasm,"i32u-rem",I32,I32_remu,I32 left,I32 right)
        {
            return (I32)((U32)left % (U32)right);
        }
        DEFINE_INTRINSIC_FUNCTION(asm2wasm,"i32s-rem",I32,I32_rems,I32 left,I32 right)
        {
            return left % right;
        }
        DEFINE_INTRINSIC_FUNCTION(asm2wasm,"i32u-div",I32,I32_divu,I32 left,I32 right)
        {
            return (I32)((U32)left / (U32)right);
        }
        DEFINE_INTRINSIC_FUNCTION(asm2wasm,"i32s-div",I32,I32_divs,I32 left,I32 right)
        {
            return left / right;
        }

        //-------------------------------------------------------------------------------
        // Keto method definitions
        //-------------------------------------------------------------------------------
        keto::wavm_common::WavmSessionTransactionPtr castToTransactionSession(keto::wavm_common::WavmSessionPtr wavmSessionPtr) {
            if (wavmSessionPtr->getSessionType() == keto::wavm_common::Constants::SESSION_TYPES::TRANSACTION)  {
                return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransaction>(wavmSessionPtr);
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidWavmSessionTypeException());
        }

        keto::wavm_common::WavmSessionHttpPtr castToTHttpSession(keto::wavm_common::WavmSessionPtr wavmSessionPtr) {
            if (wavmSessionPtr->getSessionType() == keto::wavm_common::Constants::SESSION_TYPES::HTTP)  {
                return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionHttp>(wavmSessionPtr);
            }
            BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidWavmSessionTypeException());
        }

        I32 createCstringBuf(Runtime::MemoryInstance* memory, const std::string& returnString ) {
            int length = returnString.size() + 1;

            I32 base = coerce32bitAddress(allocateMemory(memory,length));
            memset(Runtime::memoryArrayPtr<U8>(memory,base,length),0,length);
            memcpy(Runtime::memoryArrayPtr<U8>(memory,base,length),returnString.data(),
                   returnString.size());
            return base;
        }

        // type script method mappings
        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__console",void,keto_console,I32 msg)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(memory,msg);
            return;
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__log",void,keto_log,I32 level,I32 msg)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(memory,msg);
            keto::wavm_common::WavmUtils::log(U32(level),msgString);
            return;
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getFeeAccount",I32,keto_getFeeAccount)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string accountHash = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getFeeAccount();
            return createCstringBuf(memory,accountHash);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getAccount",I32,keto_getAccount)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getAccount();
            return createCstringBuf(memory,account);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getTransaction",I32,keto_getTransaction)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string transaction = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransaction();
            return createCstringBuf(memory,transaction);
        }

        // rdf methods
        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_executeQuery",I64,keto_rdf_executeQuery,I32 type, I32 query)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string queryStr = keto::wavm_common::WavmUtils::readCString(memory,query);
            std::string typeStr = keto::wavm_common::WavmUtils::readCString(memory,type);

            return (I64)(long)
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->executeQuery(typeStr,queryStr);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryHeaderCount",I64,keto_rdf_getQueryHeaderCount,I64 id)
        {
            return (I64)(long)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryHeaderCount(id);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryHeader",I32,keto_rdf_getQueryHeader,I64 id, I64 index)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string header = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryHeader(id,index);
            return createCstringBuf(memory,header);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryString",I32,keto_rdf_getQueryStringValue,I64 id, I64 index, I64 headerNumber)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string value = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryStringValue(id,index,headerNumber);
            return createCstringBuf(memory,value);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryStringByKey",I32,keto_rdf_getQueryStringByKey,I64 id, I64 index, I32 name)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(memory,name);
            std::string value = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryStringValue(id,index,nameString);
            return createCstringBuf(memory,value);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryLong",I64,keto_rdf_getQueryLong,I64 id, I64 index, I64 headerNumber)
        {
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryLongValue(id,index,headerNumber);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryLongByKey",I64,keto_rdf_getQueryLongByKey,I64 id, I64 index, I32 name)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(memory,name);
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryLongValue(id,index,nameString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryFloat",I32,keto_rdf_getQueryFloat,I64 id, I64 index, I64 headerNumber)
        {
            return (I32)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryFloatValue(id,index,headerNumber);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getQueryFloatByKey",I32,keto_rdf_getQueryFloatByKey,I64 id, I64 index, I32 name)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(memory,name);
            return (I32)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryFloatValue(id,index,nameString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__rdf_getRowCount",I64,keto_rdf_getRowCount,I64 id)
        {
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getRowCount(id);
        }


        // transaction methods
        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getTransactionValue",I64,keto_getTransactionValue)
        {
            return (I64)(long)castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransactionValue();
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getTotalFeeValue",I64,keto_getTotalFeeValue, I64 minimum)
        {
            return (I64)(long)castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTotalTransactionFee(minimum);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getFeeValue",I64,keto_getFeeValue, I64 minimum)
        {
            return (I64)(long)castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransactionFee(minimum);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getRequestModelTransactionValue",I64,keto_getRequestModelTransactionValue,I32 accountModel,I32 transactionValueModel)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(memory,transactionValueModel);

            return (I64)(long)castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    getRequestModelTransactionValue(accountModelString,transactionValueModelString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__createDebitEntry",void,keto_createDebitEntry,I32 accountId, I32 name, I32 description, I32 accountModel,I32 transactionValueModel,I64 value)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string accountIdString = keto::wavm_common::WavmUtils::readCString(memory,accountId);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(memory,name);
            std::string descriptionString = keto::wavm_common::WavmUtils::readCString(memory,description);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(memory,transactionValueModel);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            createDebitEntry(accountIdString, nameString, descriptionString, accountModelString,transactionValueModelString,(U64)value);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__createCreditEntry",void,keto_createCreditEntry, I32 accountId, I32 name, I32 description, I32 accountModel,I32 transactionValueModel,I64 value)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string accountIdString = keto::wavm_common::WavmUtils::readCString(memory,accountId);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(memory,name);
            std::string descriptionString = keto::wavm_common::WavmUtils::readCString(memory,description);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(memory,transactionValueModel);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            createCreditEntry(accountIdString, nameString, descriptionString, accountModelString,transactionValueModelString,(U64)value);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getRequestStringValue",I32,keto_getRequestStringValue,I32 subject,I32 predicate)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            std::string requestString = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRequestStringValue(subjectString,predicateString);
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__setResponseStringValue",void,keto_setResponseStringValue,I32 subject,I32 predicate,I32 value)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);
            std::string valueString = keto::wavm_common::WavmUtils::readCString(memory,value);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
               setResponseStringValue(subjectString,predicateString,valueString);
        }


        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getRequestLongValue",I64,keto_getRequestLongValue,I32 subject,I32 predicate)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            return (I64) castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            getRequestLongValue(subjectString,predicateString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__setResponseLongValue",void,keto_setResponseLongValue,I32 subject,I32 predicate, I64 value)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            setResponseLongValue(subjectString,predicateString,(U64)value);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getResponseFloatValue",I32,keto_getRequestFloatValue,I32 subject,I32 predicate)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            return (I32)castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    getRequestFloatValue(subjectString,predicateString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__setResponseFloatValue",void,keto_setResponseFloatValue,I32 subject,I32 predicate,I32 value)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    setResponseFloatValue(subjectString,predicateString,(U32)value);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__getRequestBooleanValue",I32,keto_getRequestBooleanValue,I32 subject,I32 predicate)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            return (bool) castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRequestBooleanValue(subjectString,predicateString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__setResponseBooleanValue",void,keto_setResponseBooleanValue,I32 subject,I32 predicate,I32 value)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(memory,predicate);

            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    setResponseBooleanValue(subjectString,predicateString,(bool)value);
        }

        //
        // http session wrapping
        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getNumberOfRoles",I64,keto_http_getNumberOfRoles)
        {
            return (I64)(long)castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getNumberOfRoles();
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getRole",I32,keto_http_getRole,I64 index)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRole(index);
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getTargetUri",I32,keto_http_getTargetUri)
        {

            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTargetUri();
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getQuery",I32,keto_http_getQuery)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getQuery();
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getMethod",I32,keto_http_getMethod)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getMethod();
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getBody",I32,keto_http_getBody)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getBody();
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getNumberOfParameters",I64,keto_http_getNumberOfParameters)
        {
            return (I64)(long)castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getNumberOfParameters();
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getParameterKey",I32,keto_http_getParameterKey,I64 index)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getParameterKey(index);
            return createCstringBuf(memory,requestString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_getParameter",I32,keto_http_getParameter,I32 key)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string keyString = keto::wavm_common::WavmUtils::readCString(memory,key);
            std::string requestString = castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getParameter(keyString);
            return createCstringBuf(memory,requestString);
        }

        // response methods
        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_setStatus",void,keto_http_setStatus,I64 statusCode)
        {
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setStatus(statusCode);
        }


        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_setContentType",void,keto_http_setContentType,I32 contentType)
        {

            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string contentTypeString = keto::wavm_common::WavmUtils::readCString(memory,contentType);
            castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setContentType(contentTypeString);
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(keto,"__http_setBody",void,keto_http_setBody,I32 body)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string bodyString = keto::wavm_common::WavmUtils::readCString(memory,body);
            castToTHttpSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setBody(bodyString);
        }

        // C/CPP method mappings
        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(env,"console",void,c_console,I32 msg)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(memory,msg);
            return;
        }

        DEFINE_INTRINSIC_FUNCTION_WITH_MEM_AND_TABLE(env,"log",void,c_log,I64 level,I32 msg)
        {
            Runtime::MemoryInstance* memory = getMemoryFromRuntimeData(contextRuntimeData,defaultMemoryId.id);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(memory,msg);
            keto::wavm_common::WavmUtils::log(U32(level),msgString);
            return;
        }

        // transaction builder api



        //-------------------------------------------------------------------------------
        // End of Keto method definitions
        //-------------------------------------------------------------------------------

        EMSCRIPTEN_API keto::Emscripten::Instance* instantiate(Runtime::Compartment* compartment)
        {
            keto::Emscripten::Instance* instance = new keto::Emscripten::Instance;
            instance->env = Intrinsics::instantiateModule(compartment,INTRINSIC_MODULE_REF(env));
            instance->asm2wasm = Intrinsics::instantiateModule(compartment,INTRINSIC_MODULE_REF(asm2wasm));
            instance->global = Intrinsics::instantiateModule(compartment,INTRINSIC_MODULE_REF(global));
            instance->keto = Intrinsics::instantiateModule(compartment,INTRINSIC_MODULE_REF(keto));

            MutableGlobals& mutableGlobals = Runtime::memoryRef<MutableGlobals>(
                    emscriptenMemory.getInstance(instance->env),
                    MutableGlobals::address);

            mutableGlobals._stderr = (U32)ioStreamVMHandle::StdErr;
            mutableGlobals._stdin = (U32)ioStreamVMHandle::StdIn;
            mutableGlobals._stdout = (U32)ioStreamVMHandle::StdOut;

            if (!emscriptenMemoryInstance) {
                emscriptenMemoryInstance = emscriptenMemory.getInstance(instance->env);
            }

            return instance;
        }

        EMSCRIPTEN_API void initializeGlobals(Runtime::Context* context,const WAVM::IR::Module& module,Runtime::Instance* moduleInstance)
        {
            // Call the establishStackSpace function to set the Emscripten module's internal stack pointers.
            Runtime::FunctionInstance* establishStackSpace = asFunctionNullable(getInstanceExport(moduleInstance,"establishStackSpace"));
            if(establishStackSpace
            && getFunctionType(establishStackSpace) == WAVM::IR::FunctionType::get(WAVM::IR::ResultType::none,{WAVM::IR::ValueType::i32,WAVM::IR::ValueType::i64}))
            {
                std::vector<Runtime::Value> parameters =
                {
                    Runtime::Value(STACKTOP.getValue().i32),
                    Runtime::Value(STACK_MAX.getValue().i32)
                };
                Runtime::invokeFunctionChecked(context,establishStackSpace,parameters);
            }

            // Call the global initializer functions.
            for(Uptr exportIndex = 0;exportIndex < module.exports.size();++exportIndex)
            {
                const WAVM::IR::Export& functionExport = module.exports[exportIndex];
                if(functionExport.kind == WAVM::IR::ObjectKind::function && !strncmp(functionExport.name.c_str(),"__GLOBAL__",10))
                {
                    Runtime::FunctionInstance* functionInstance = asFunctionNullable(getInstanceExport(moduleInstance,functionExport.name));
                    if(functionInstance) { Runtime::invokeFunctionChecked(context,functionInstance,{}); }
                }
            }
        }

        EMSCRIPTEN_API void injectCommandArgs(keto::Emscripten::Instance* instance,const std::vector<const char*>& argStrings,std::vector<Runtime::Value>& outInvokeArgs)
        {
            U8* emscriptenMemoryBaseAdress = getMemoryBaseAddress(emscriptenMemoryInstance);

            U32* argvOffsets = (U32*)(emscriptenMemoryBaseAdress + sbrk((U32)(sizeof(U32) * (argStrings.size() + 1))));
            for(Uptr argIndex = 0;argIndex < argStrings.size();++argIndex)
            {
                auto stringSize = strlen(argStrings[argIndex])+1;
                auto stringMemory = emscriptenMemoryBaseAdress + sbrk((U32)stringSize);
                memcpy(stringMemory,argStrings[argIndex],stringSize);
                argvOffsets[argIndex] = (U32)(stringMemory - emscriptenMemoryBaseAdress);
            }
            argvOffsets[argStrings.size()] = 0;
            outInvokeArgs = {(U32)argStrings.size(), (U32)((U8*)argvOffsets - emscriptenMemoryBaseAdress) };
        }


        */
    }
}
