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
#include "WAVM/Inline/Serialization.h"
#include "WAVM/IR/Module.h"
#include "WAVM/Inline/LEB128.h"

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

using namespace WAVM;
using namespace WAVM::IR;
using namespace WAVM::Runtime;

WAVM::Intrinsics::Module* getIntrinsicModule_global();
WAVM::Intrinsics::Module* getIntrinsicModule_env();
WAVM::Intrinsics::Module* getIntrinsicModule_asm2wasm();
WAVM::Intrinsics::Module* getIntrinsicModule_emscripten_wasi_unstable();

namespace WAVM {
    namespace Emscripten {
        struct EmscriptenModuleMetadata
        {
            U32 metadataVersionMajor;
            U32 metadataVersionMinor;

            U32 abiVersionMajor;
            U32 abiVersionMinor;

            U32 backendID;
            U32 numMemoryPages;
            U32 numTableElems;
            U32 globalBaseAddress;
            U32 dynamicBaseAddress;
            U32 dynamicTopAddressAddress;
            U32 tempDoubleAddress;
            U32 standaloneWASM;
        };
    }
}

static bool loadEmscriptenMetadata(const WAVM::IR::Module& module, WAVM::Emscripten::EmscriptenModuleMetadata& outMetadata)
{
    for(const CustomSection& customSection : module.customSections)
    {
        if(customSection.name == "emscripten_metadata")
        {
            try
            {
                WAVM::Serialization::MemoryInputStream sectionStream(customSection.data.data(),
                                                               customSection.data.size());

                serializeVarUInt32(sectionStream, outMetadata.metadataVersionMajor);
                serializeVarUInt32(sectionStream, outMetadata.metadataVersionMinor);

                if(outMetadata.metadataVersionMajor != 0 || outMetadata.metadataVersionMinor < 2)
                {
                    WAVM::Log::printf(WAVM::Log::error,
                                "Unsupported Emscripten module metadata version: %u\n",
                                outMetadata.metadataVersionMajor);
                    return false;
                }

                serializeVarUInt32(sectionStream, outMetadata.abiVersionMajor);
                serializeVarUInt32(sectionStream, outMetadata.abiVersionMinor);

                serializeVarUInt32(sectionStream, outMetadata.backendID);
                serializeVarUInt32(sectionStream, outMetadata.numMemoryPages);
                serializeVarUInt32(sectionStream, outMetadata.numTableElems);
                serializeVarUInt32(sectionStream, outMetadata.globalBaseAddress);
                serializeVarUInt32(sectionStream, outMetadata.dynamicBaseAddress);
                serializeVarUInt32(sectionStream, outMetadata.dynamicTopAddressAddress);
                serializeVarUInt32(sectionStream, outMetadata.tempDoubleAddress);

                if(outMetadata.metadataVersionMinor >= 3)
                { serializeVarUInt32(sectionStream, outMetadata.standaloneWASM); }
                else
                {
                    outMetadata.standaloneWASM = 0;
                }

                return true;
            }
            catch(WAVM::Serialization::FatalSerializationException const& exception)
            {
                WAVM::Log::printf(WAVM::Log::error,
                            "Error while deserializing Emscripten metadata section: %s\n",
                            exception.message.c_str());
                return false;
            }
        }
    }

    WAVM::Log::printf(WAVM::Log::error,
                "Module did not contain Emscripten module metadata section: WAVM only supports"
                " Emscripten modules compiled with '-S EMIT_EMSCRIPTEN_METADATA=1'.\n");
    return false;
}

namespace keto {
    namespace Emscripten {

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

        struct Process : WAVM::Runtime::Resolver
        {
            WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment;

            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> wasi_unstable;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> env;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> keto;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> asm2wasm;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> global;

            WAVM::IntrusiveSharedPtr<Thread> mainThread;

            WAVM::Emscripten::EmscriptenModuleMetadata metadata;

            WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> instance;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Memory> memory;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Table> table;

            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> malloc;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> free;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> stackAlloc;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> stackSave;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> stackRestore;
            WAVM::Runtime::GCPointer<WAVM::Runtime::Function> errnoLocation;

            // A global list of running threads created by WebAssembly code.
            WAVM::Platform::Mutex threadsMutex;
            WAVM::IndexMap<WAVM::Emscripten::emabi::pthread_t, WAVM::IntrusiveSharedPtr<Thread>> threads{1, UINT32_MAX};

            std::atomic<WAVM::Emscripten::emabi::pthread_key_t> pthreadSpecificNextKey{0};

            std::atomic<WAVM::Emscripten::emabi::Address> currentLocale;

            std::vector<std::string> args;
            std::vector<std::string> envs;

            WAVM::VFS::VFD* stdIn{nullptr};
            WAVM::VFS::VFD* stdOut{nullptr};
            WAVM::VFS::VFD* stdErr{nullptr};

            ~Process();

            bool resolve(const std::string& moduleName,
                         const std::string& exportName,
                         WAVM::IR::ExternType type,
                         WAVM::Runtime::Object*& outObject) override;
        };


        bool resizeHeap(Process* process, U32 desiredNumBytes)
        {
            const Uptr desiredNumPages
                    = (Uptr(desiredNumBytes) + WAVM::IR::numBytesPerPage - 1) / WAVM::IR::numBytesPerPage;
            const Uptr currentNumPages = WAVM::Runtime::getMemoryNumPages(process->memory);
            if(desiredNumPages > currentNumPages)
            {
                if(!WAVM::Runtime::growMemory(process->memory, desiredNumPages - currentNumPages))
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


        inline Emscripten::Process* getEmscriptenInstance(
                WAVM::Runtime::ContextRuntimeData* contextRuntimeData);

        enum class ioStreamVMHandle
        {
            StdIn = 0,
            StdOut = 1,
            StdErr = 2,
        };
    }
}

namespace WAVM {
    namespace Emscripten {
        // Metadata from the emscripten_metadata user section of a module emitted by Emscripten.

        WAVM::Intrinsics::Module* getIntrinsicModule_envThreads();

        // C/CPP method mappings
        WAVM_DEFINE_INTRINSIC_FUNCTION(env,"console",void,c_console,WAVM::Emscripten::emabi::Address msg)
        {
            keto::Emscripten::Process* instance = keto::Emscripten::getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            return;
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(env,"log",void,c_log,I64 level,WAVM::Emscripten::emabi::Address msg)
        {
            keto::Emscripten::Process* instance = keto::Emscripten::getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            keto::wavm_common::WavmUtils::log(U32(level),msgString);
            return;
        }

    }
}


namespace keto {
    namespace Emscripten
    {


        WAVM::Emscripten::emabi::Address dynamicAlloc(Emscripten::Process* process,
                                                Context* context,
                                                WAVM::Emscripten::emabi::Size numBytes)
        {
            static FunctionType mallocSignature({ValueType::i32}, {ValueType::i32});

            UntaggedValue args[1] = {numBytes};
            UntaggedValue results[1];
            invokeFunction(context, process->malloc, mallocSignature, args, results);

            return results[0].u32;
        }


        inline Emscripten::Process* getEmscriptenInstance(
                WAVM::Runtime::ContextRuntimeData* contextRuntimeData)
        {
            auto process = (Emscripten::Process*)getUserData(
                    getCompartmentFromContextRuntimeData(contextRuntimeData));
            WAVM_ASSERT(process);
            WAVM_ASSERT(process->memory);
            return process;
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
            Emscripten::Process* process;
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

            Thread(Emscripten::Process* process,
                   WAVM::Runtime::Context* inContext,
                   WAVM::Runtime::Function* inEntryFunction,
                   I32 inArgument)
                    : process(process)
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
                    getInstanceExport(thread->process->instance, "establishStackSpace"));
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

        void joinAllThreads(Process& process)
        {
            while(true)
            {
                WAVM::Platform::Mutex::Lock threadsLock(process.threadsMutex);

                if(!process.threads.size()) { break; }
                auto it = process.threads.begin();
                WAVM_ASSERT(it != process.threads.end());

                WAVM::Emscripten::emabi::pthread_t threadId = it.getIndex();
                WAVM::IntrusiveSharedPtr<Thread> thread = std::move(*it);
                process.threads.removeOrFail(threadId);

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
        Process::~Process()
        {
            // Instead of allowing an Instance to live on until all its threads exit, wait for all threads
            // to exit before destroying the Instance.
            joinAllThreads(*this);
        }

        bool Process::resolve(const std::string& moduleName,
                                           const std::string& exportName,
                                           WAVM::IR::ExternType type,
                                           WAVM::Runtime::Object*& outObject)
        {
            WAVM::Runtime::Instance* intrinsicInstance = nullptr;
            if(moduleName == "env")
            {
                intrinsicInstance = env;
            }
            else if(moduleName == "asm2wasm")
            {
                intrinsicInstance = asm2wasm;
            }
            else if(moduleName == "global")
            {
                intrinsicInstance = global;
            }
            else if(moduleName == "wasi_unstable")
            {
                intrinsicInstance = wasi_unstable;
            }
            else if(moduleName == "keto")
            {
                intrinsicInstance = keto;
            }

            if(intrinsicInstance)
            {
                outObject = getInstanceExport(intrinsicInstance, exportName);
                if(outObject)
                {
                    if(isA(outObject, type)) {
                        return true;
                    }
                    else
                    {
                        //KETO_LOG_ERROR << "The object was not found";
                        WAVM::Log::printf(WAVM::Log::error,
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

        WAVM::Emscripten::emabi::Address createCstringBuf(Process* process, Context* context, const std::string& returnString ) {
            int length = returnString.size() + 1;

            WAVM::Emscripten::emabi::Address base = dynamicAlloc(process, context,length);
            memset(WAVM::Runtime::memoryArrayPtr<U8>(process->memory,base,length),0,length);
            memcpy(WAVM::Runtime::memoryArrayPtr<U8>(process->memory,base,length),returnString.data(),
                   returnString.size());
            return base;
        }

        // type script method mappings
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__console",void,keto_console,WAVM::Emscripten::emabi::Address msg)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            return;
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__log",void,keto_log,I32 level,WAVM::Emscripten::emabi::Address msg)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string msgString = keto::wavm_common::WavmUtils::readCString(instance->memory,msg);
            keto::wavm_common::WavmUtils::log(U32(level),msgString);
            return;
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getFeeAccount",WAVM::Emscripten::emabi::Address,keto_getFeeAccount)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountHash = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getFeeAccount();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),accountHash);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getAccount",WAVM::Emscripten::emabi::Address,keto_getAccount)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getAccount();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),account);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getTransaction",WAVM::Emscripten::emabi::Address,keto_getTransaction)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string transaction = castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTransaction();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),transaction);
        }

        // rdf methods
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_executeQuery",I64,keto_rdf_executeQuery,WAVM::Emscripten::emabi::Address type, WAVM::Emscripten::emabi::Address query)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string queryStr = keto::wavm_common::WavmUtils::readCString(instance->memory,query);
            std::string typeStr = keto::wavm_common::WavmUtils::readCString(instance->memory,type);

            return (I64)(long)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->executeQuery(typeStr,queryStr);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryHeaderCount",I64,keto_rdf_getQueryHeaderCount,I64 id)
        {
            return (I64)(long)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryHeaderCount(id);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryHeader",WAVM::Emscripten::emabi::Address,keto_rdf_getQueryHeader,I64 id, I64 index)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string header = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryHeader(id,index);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),header);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryString",WAVM::Emscripten::emabi::Address,keto_rdf_getQueryStringValue,I64 id, I64 index, I64 headerNumber)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string value = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryStringValue(id,index,headerNumber);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryStringByKey",WAVM::Emscripten::emabi::Address,keto_rdf_getQueryStringByKey,I64 id, I64 index, WAVM::Emscripten::emabi::Address name)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            std::string value = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryStringValue(id,index,nameString);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryLong",I64,keto_rdf_getQueryLong,I64 id, I64 index, I64 headerNumber)
        {
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryLongValue(id,index,headerNumber);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryLongByKey",I64,keto_rdf_getQueryLongByKey,I64 id, I64 index, WAVM::Emscripten::emabi::Address name)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            return (I64)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryLongValue(id,index,nameString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryFloat",I32,keto_rdf_getQueryFloat,I64 id, I64 index, I64 headerNumber)
        {
            return (I32)keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getQueryFloatValue(id,index,headerNumber);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__rdf_getQueryFloatByKey",I32,keto_rdf_getQueryFloatByKey,I64 id, I64 index, WAVM::Emscripten::emabi::Address name)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
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

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestModelTransactionValue",I64,keto_getRequestModelTransactionValue,WAVM::Emscripten::emabi::Address accountModel,WAVM::Emscripten::emabi::Address transactionValueModel)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,transactionValueModel);

            return (I64)(long)castToTransactionSession(
            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            getRequestModelTransactionValue(accountModelString,transactionValueModelString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__createDebitEntry",void,keto_createDebitEntry,WAVM::Emscripten::emabi::Address accountId, WAVM::Emscripten::emabi::Address name, WAVM::Emscripten::emabi::Address description, WAVM::Emscripten::emabi::Address accountModel,WAVM::Emscripten::emabi::Address transactionValueModel,I64 value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountIdString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountId);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            std::string descriptionString = keto::wavm_common::WavmUtils::readCString(instance->memory,description);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,transactionValueModel);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->createDebitEntry(
                    accountIdString, nameString, descriptionString, accountModelString,transactionValueModelString,(U64)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__createCreditEntry",void,keto_createCreditEntry, WAVM::Emscripten::emabi::Address accountId, WAVM::Emscripten::emabi::Address name, WAVM::Emscripten::emabi::Address description, WAVM::Emscripten::emabi::Address accountModel,WAVM::Emscripten::emabi::Address transactionValueModel,I64 value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountIdString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountId);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);
            std::string descriptionString = keto::wavm_common::WavmUtils::readCString(instance->memory,description);
            std::string accountModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountModel);
            std::string transactionValueModelString = keto::wavm_common::WavmUtils::readCString(instance->memory,transactionValueModel);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->createCreditEntry(
                    accountIdString, nameString, descriptionString, accountModelString,transactionValueModelString,(U64)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestStringValue",WAVM::Emscripten::emabi::Address,keto_getRequestStringValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            std::string requestString = castToTransactionSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRequestStringValue(subjectString,predicateString);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseStringValue",void,keto_setResponseStringValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate,WAVM::Emscripten::emabi::Address value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);
            std::string valueString = keto::wavm_common::WavmUtils::readCString(instance->memory,value);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setResponseStringValue(
                    subjectString,predicateString,valueString);
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestLongValue",I64,keto_getRequestLongValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            return (I64) castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRequestLongValue(
                            subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseLongValue",void,keto_setResponseLongValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate, I64 value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setResponseLongValue(
                            subjectString,predicateString,(U64)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getResponseFloatValue",I32,keto_getRequestFloatValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            return (I32)castToTransactionSession(
            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
            getRequestFloatValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseFloatValue",void,keto_setResponseFloatValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate,WAVM::Emscripten::emabi::Address value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->
            getWavmSession())->setResponseFloatValue(subjectString,predicateString,(U32)value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getRequestBooleanValue",I32,keto_getRequestBooleanValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            return (bool) castToTransactionSession(keto::wavm_common::WavmSessionManager::getInstance()->
            getWavmSession())->getRequestBooleanValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__setResponseBooleanValue",void,keto_setResponseBooleanValue,WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate,WAVM::Emscripten::emabi::Address value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
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

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getRole",WAVM::Emscripten::emabi::Address,keto_http_getRole,I64 index)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getRole(index);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getTargetUri",WAVM::Emscripten::emabi::Address,keto_http_getTargetUri)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getTargetUri();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getQuery",WAVM::Emscripten::emabi::Address,keto_http_getQuery)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getQuery();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getMethod",WAVM::Emscripten::emabi::Address,keto_http_getMethod)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getMethod();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getBody",WAVM::Emscripten::emabi::Address,keto_http_getBody)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getBody();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getNumberOfParameters",I64,keto_http_getNumberOfParameters)
        {
            return (I64)(long)castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getNumberOfParameters();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getParameterKey",WAVM::Emscripten::emabi::Address,keto_http_getParameterKey,I64 index)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getParameterKey(index);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_getParameter",WAVM::Emscripten::emabi::Address,keto_http_getParameter,WAVM::Emscripten::emabi::Address key)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string keyString = keto::wavm_common::WavmUtils::readCString(instance->memory,key);
            std::string requestString = castToTHttpSession(
                keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->getParameter(keyString);
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),requestString);
        }

        // response methods
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_setStatus",void,keto_http_setStatus,I64 statusCode)
        {
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setStatus(statusCode);
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_setContentType",void,keto_http_setContentType,WAVM::Emscripten::emabi::Address contentType)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string contentTypeString = keto::wavm_common::WavmUtils::readCString(instance->memory,contentType);
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setContentType(contentTypeString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__http_setBody",void,keto_http_setBody,WAVM::Emscripten::emabi::Address body)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string bodyString = keto::wavm_common::WavmUtils::readCString(instance->memory,body);
            castToTHttpSession(keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->setBody(bodyString);
        }

        // transaction builder api

        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        //
        // The Emscripten
        //
        //--------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------
        bool isEmscriptenModule(const WAVM::IR::Module& irModule) {
            for(const WAVM::IR::CustomSection& customSection : irModule.customSections)
            {
                if(customSection.name == "emscripten_metadata") { return true; }
            }

            return false;
        }


        std::shared_ptr<Emscripten::Process> createProcess(Compartment* compartment,
                                                                       std::vector<std::string>&& inArgs,
                                                                       std::vector<std::string>&& inEnvs,
                                                                       WAVM::VFS::VFD* stdIn,
                                                                       WAVM::VFS::VFD* stdOut,
                                                                       WAVM::VFS::VFD* stdErr)
        {
            std::shared_ptr<Process> process = std::make_shared<Process>();

            process->args = std::move(inArgs);
            process->envs = std::move(inEnvs);
            process->stdIn = stdIn;
            process->stdOut = stdOut;
            process->stdErr = stdErr;

            process->env = WAVM::Intrinsics::instantiateModule(
                    compartment,
                    {WAVM_INTRINSIC_MODULE_REF(env), WAVM::Emscripten::getIntrinsicModule_envThreads()},
                    "env");
            process->keto = WAVM::Intrinsics::instantiateModule(
                    compartment,
                    {WAVM_INTRINSIC_MODULE_REF(keto)},
                    "keto");
            process->asm2wasm = WAVM::Intrinsics::instantiateModule(
                    compartment, {WAVM_INTRINSIC_MODULE_REF(asm2wasm)}, "asm2wasm");
            process->global
                    = WAVM::Intrinsics::instantiateModule(compartment, {WAVM_INTRINSIC_MODULE_REF(global)}, "global");
            process->wasi_unstable = WAVM::Intrinsics::instantiateModule(
                    compartment, {WAVM_INTRINSIC_MODULE_REF(emscripten_wasi_unstable)}, "wasi_unstable");

            process->compartment = compartment;

            setUserData(compartment, process.get());

            return process;
        }

        bool initializeProcess(Process& process,
                               WAVM::Runtime::Context* context,
                               const WAVM::IR::Module& module,
                               WAVM::Runtime::Instance* instance) {

            process.instance = instance;

            if (keto::Emscripten::isEmscriptenModule(module)) {
                // Read the module metadata.
                if (!loadEmscriptenMetadata(module, process.metadata)) {
                    WAVM::Log::printf(WAVM::Log::output,
                                      "The em script meta data was found and could be loaded");
                    return false;
                }

                // Check the ABI version used by the module.
                if (process.metadata.abiVersionMajor != 0) {
                    WAVM::Log::printf(WAVM::Log::error,
                                      "Unsupported Emscripten ABI major version (%u)\n",
                                      process.metadata.abiVersionMajor);
                    return false;
                }

                // Check whether the module was compiled as "standalone".
                if (!process.metadata.standaloneWASM) {
                    WAVM::Log::printf(WAVM::Log::error,
                                      "WAVM only supports Emscripten modules compiled with '-s STANDALONE_WASM=1'.");
                    return false;
                }

                // Find the various Emscripten ABI objects exported by the module.
                process.memory = asMemoryNullable(getInstanceExport(instance, "memory"));
                if (!process.memory) {
                    WAVM::Log::printf(WAVM::Log::error, "Emscripten module does not export memory.\n");
                    return false;
                }

                process.table = asTableNullable(getInstanceExport(instance, "table"));

                process.malloc = getTypedInstanceExport(
                        instance, "malloc", FunctionType({ValueType::i32}, {ValueType::i32}));
                if (!process.malloc) {
                    process.malloc = getTypedInstanceExport(
                            instance, "_malloc", FunctionType({ValueType::i32}, {ValueType::i32}));
                    if (!process.malloc) {
                        WAVM::Log::printf(WAVM::Log::error, "Emscripten module does not export malloc.\n");
                        return false;
                    }
                }
                process.free = getTypedInstanceExport(
                        instance, "free", FunctionType({ValueType::i32}, {ValueType::i32}));

                process.stackAlloc = getTypedInstanceExport(
                        instance, "stackAlloc", FunctionType({ValueType::i32}, {ValueType::i32}));
                process.stackSave
                        = getTypedInstanceExport(instance, "stackSave", FunctionType({ValueType::i32}, {}));
                process.stackRestore
                        = getTypedInstanceExport(instance, "stackRestore", FunctionType({}, {ValueType::i32}));
                process.errnoLocation
                        = getTypedInstanceExport(instance, "__errno_location", FunctionType({ValueType::i32}, {}));
            } else {
                // Find the various Emscripten ABI objects exported by the module.
                process.memory = asMemoryNullable(getInstanceExport(instance, "memory"));
                if (!process.memory) {
                    WAVM::Log::printf(WAVM::Log::error, "Emscripten module does not export memory.\n");
                    return false;
                }

                process.table = asTableNullable(getInstanceExport(instance, "table"));
                process.malloc = getTypedInstanceExport(
                        instance, "malloc", FunctionType({ValueType::i32}, {ValueType::i32}));
                if (!process.malloc) {
                    process.malloc = getTypedInstanceExport(
                            instance, "_malloc", FunctionType({ValueType::i32}, {ValueType::i32}));
                    if (!process.malloc) {
                        WAVM::Log::printf(WAVM::Log::error, "Failed to find the malloc function.\n");
                    }
                }
                process.free = getTypedInstanceExport(
                        instance, "free", FunctionType({ValueType::i32}, {ValueType::i32}));
                if (!process.free) {
                    process.free = getTypedInstanceExport(
                            instance, "_free", FunctionType({}, {ValueType::i32}));
                    if (!process.free) {
                        WAVM::Log::printf(WAVM::Log::error, "Failed to find the free function.\n");
                    }
                }
                process.stackAlloc = getTypedInstanceExport(
                        instance, "stackAlloc", FunctionType({ValueType::i32}, {ValueType::i32}));
                process.stackSave
                        = getTypedInstanceExport(instance, "stackSave", FunctionType({ValueType::i32}, {}));
                process.stackRestore
                        = getTypedInstanceExport(instance, "stackRestore", FunctionType({}, {ValueType::i32}));
                process.errnoLocation
                        = getTypedInstanceExport(instance, "__errno_location", FunctionType({ValueType::i32}, {}));
            }

            // Create an Emscripten "main thread" and associate it with this context.
            process.mainThread = new Emscripten::Thread(&process, context, nullptr, 0);
            setUserData(context, process.mainThread);

            // TODO
            process.mainThread->stackAddress = 0;
            process.mainThread->numStackBytes = 0;

            // Initialize the Emscripten "thread local" state.
            initThreadLocals(process.mainThread);

            return true;
        }

        WAVM::Runtime::Resolver& getInstanceResolver(Process& process) { return process; }

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

        EMSCRIPTEN_API keto::Emscripten::Process* instantiate(Runtime::Compartment* compartment)
        {
            keto::Emscripten::Process* instance = new keto::Emscripten::Process;
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

        EMSCRIPTEN_API void injectCommandArgs(keto::Emscripten::Process* instance,const std::vector<const char*>& argStrings,std::vector<Runtime::Value>& outInvokeArgs)
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
