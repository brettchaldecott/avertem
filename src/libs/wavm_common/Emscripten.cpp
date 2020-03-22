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


        /*bool resizeHeap(Process* process, U32 desiredNumBytes)
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
        }*/


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

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getContractName",WAVM::Emscripten::emabi::Address,keto_getContractName)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string name = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getContractName();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),name);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getContractHash",WAVM::Emscripten::emabi::Address,keto_getContractHash)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string hash = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getContractHash();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),hash);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getContractOwner",WAVM::Emscripten::emabi::Address,keto_getContractOwner)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getContractOwner();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),account);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getAccount",WAVM::Emscripten::emabi::Address,keto_getAccount)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getAccount();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),account);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getDebitAccount",WAVM::Emscripten::emabi::Address,keto_getDebitAccount)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getDebitAccount();
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),account);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__getCreditAccount",WAVM::Emscripten::emabi::Address,keto_getCreditAccount)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string account = keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession()->getCreditAccount();
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


        // child transaction methods
        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_createTransaction",I32,keto_transaction_createTransaction)
        {
            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                castToTransactionSession(
                        keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                        createChildTransaction();
            wavmSessionTransactionBuilderPtr->setTransactionSignator(keto::server_common::ServerInfo::getInstance()->getAccountHash());
            wavmSessionTransactionBuilderPtr->setCreatorId(keto::server_common::ServerInfo::getInstance()->getAccountHash());
            return wavmSessionTransactionBuilderPtr->getId();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getTransactionSignator",WAVM::Emscripten::emabi::Address,keto_transaction_getTransactionSignator, I32 transactionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);

            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    wavmSessionTransactionBuilderPtr->getTransactionSignator().getHash(keto::common::StringEncoding::HEX));
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getCreatorId",WAVM::Emscripten::emabi::Address,keto_transaction_getCreatorId, I32 transactionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);

            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    wavmSessionTransactionBuilderPtr->getCreatorId().getHash(keto::common::StringEncoding::HEX));
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getSourceAccount",WAVM::Emscripten::emabi::Address,keto_transaction_getSourceAccount, I32 transactionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);

            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    wavmSessionTransactionBuilderPtr->getSourceAccount().getHash(keto::common::StringEncoding::HEX));
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setSourceAccount",void,keto_transaction_setSourceAccount, I32 transactionId, WAVM::Emscripten::emabi::Address accountHash)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountHashString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountHash);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);
            keto::asn1::HashHelper hashHelper(accountHashString,keto::common::StringEncoding::HEX);
            wavmSessionTransactionBuilderPtr->setSourceAccount(hashHelper);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getTargetAccount",WAVM::Emscripten::emabi::Address,keto_transaction_getTargetAccount, I32 transactionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);

            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    wavmSessionTransactionBuilderPtr->getTargetAccount().getHash(keto::common::StringEncoding::HEX));
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setTargetAccount",void,keto_transaction_setTargetAccount, I32 transactionId, WAVM::Emscripten::emabi::Address accountHash)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string accountHashString = keto::wavm_common::WavmUtils::readCString(instance->memory,accountHash);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);
            keto::asn1::HashHelper hashHelper(accountHashString,keto::common::StringEncoding::HEX);
            wavmSessionTransactionBuilderPtr->setTargetAccount(hashHelper);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getParent",WAVM::Emscripten::emabi::Address,keto_transaction_getParent, I32 transactionId, I32 actionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);

            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    wavmSessionTransactionBuilderPtr->getParent().getHash(keto::common::StringEncoding::HEX));
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_createTransactionAction",I32,keto_transaction_createTransactionAction, I32 transactionId, WAVM::Emscripten::emabi::Address modelType)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string modelTypeString = keto::wavm_common::WavmUtils::readCString(instance->memory,modelType);

            keto::wavm_common::WavmSessionTransactionBuilderPtr wavmSessionTransactionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId);
            return wavmSessionTransactionBuilderPtr->createAction(modelTypeString)->getId();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getActionContractName",WAVM::Emscripten::emabi::Address,keto_transaction_getActionContractName, I32 transactionId, I32 actionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);

            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    actionBuilderPtr->getContractName());
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setActionContractName",void,keto_transaction_setActionContractName, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address name)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string nameString = keto::wavm_common::WavmUtils::readCString(instance->memory,name);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);
            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            actionBuilderPtr->setContractName(nameString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getActionContract",WAVM::Emscripten::emabi::Address,keto_transaction_getActionContract, I32 transactionId, I32 actionId)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);

            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                                    actionBuilderPtr->getContract().getHash(keto::common::StringEncoding::HEX));
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setActionContract",void,keto___transaction_setActionContract, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address contract)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string contractString = keto::wavm_common::WavmUtils::readCString(instance->memory,contract);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);
            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            actionBuilderPtr->setContract(keto::asn1::HashHelper(contractString,keto::common::StringEncoding::HEX));
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getRequestStringValue",WAVM::Emscripten::emabi::Address,keto_transaction_getRequestStringValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);

            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            return createCstringBuf(instance,getContextFromRuntimeData(contextRuntimeData),
                    std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->getRequestStringValue(subjectString,predicateString));
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setRequestStringValue",void,keto_transaction_setRequestStringValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate, WAVM::Emscripten::emabi::Address value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);
            std::string valueString = keto::wavm_common::WavmUtils::readCString(instance->memory,value);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);
            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->setRequestStringValue(subjectString,predicateString,valueString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getRequestLongValue",I64,keto_transaction_getRequestLongValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);

            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->getRequestLongValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setRequestLongValue",void,keto_transaction_setRequestLongValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate, I64 value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);
            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->setRequestLongValue(subjectString,predicateString,value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getRequestFloatValue",I32,keto_transaction_getRequestFloatValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);

            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->getRequestFloatValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setRequestFloatValue",void,keto_transaction_setRequestFloatValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate, I32 value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);
            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->setRequestFloatValue(subjectString,predicateString,value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getRequestBooleanValue",I32,keto_transaction_getRequestBooleanValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);

            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            return std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->getRequestBooleanValue(subjectString,predicateString);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setRequestBooleanValue",void,keto_transaction_setRequestBooleanValue, I32 transactionId, I32 actionId, WAVM::Emscripten::emabi::Address subject,WAVM::Emscripten::emabi::Address predicate, I32 value)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string subjectString = keto::wavm_common::WavmUtils::readCString(instance->memory,subject);
            std::string predicateString = keto::wavm_common::WavmUtils::readCString(instance->memory,predicate);

            keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionActionBuilderPtr actionBuilderPtr =
                    castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getAction(actionId);
            if (actionBuilderPtr->getModelType() != keto::wavm_common::Constants::TRANSACTION_BUILDER::MODEL::RDF) {
                BOOST_THROW_EXCEPTION(keto::wavm_common::InvalidActionModelRequest());
            }
            std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransactionBuilder::WavmSessionRDFModelBuilder>(actionBuilderPtr->getModel())->setRequestBooleanValue(subjectString,predicateString,value);
        }


        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_getTransactionValue",I64,keto_transaction_getTransactionValue, I32 transactionId)
        {
            //Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            return castToTransactionSession(
                            keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                            getChildTransaction(transactionId)->getValue();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_setTransactionValue",void,keto_transaction_setTransactionValue, I32 transactionId, I64 value)
        {
            //Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);

            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    getChildTransaction(transactionId)->setValue(value);
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_submit",void,keto_transaction_submit, I32 transactionId)
        {
            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    getChildTransaction(transactionId)->submit();
        }

        WAVM_DEFINE_INTRINSIC_FUNCTION(keto,"__transaction_submitWithStatus",void,keto_transaction_submitWithStatus, I32 transactionId, WAVM::Emscripten::emabi::Address status)
        {
            Emscripten::Process* instance = getEmscriptenInstance(contextRuntimeData);
            std::string statusString = keto::wavm_common::WavmUtils::readCString(instance->memory,status);
            castToTransactionSession(
                    keto::wavm_common::WavmSessionManager::getInstance()->getWavmSession())->
                    getChildTransaction(transactionId)->submitWithStatus(statusString);
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



    }
}
