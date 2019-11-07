/**
 * This is taken from the WAVM base project.
 */

#pragma once

#ifndef KETO_EMSCRIPTEN_API
#define EMSCRIPTEN_API DLL_IMPORT

#include <vector>

#include "WAVM/Runtime/Runtime.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Emscripten/Emscripten.h"
#include "WAVM/Inline/IntrusiveSharedPtr.h"
#include "WAVM/Platform/Mutex.h"
#include "WAVM/Inline/IndexMap.h"
#include "WAVM/Inline/BasicTypes.h"

#include "keto/obfuscate/MetaString.hpp"


using namespace WAVM;
//namespace IR { struct Module; }
//namespace Runtime { struct Instance; struct Context; struct Compartment; }

namespace keto {

    namespace Emscripten
    {
        /*
        //using namespace Runtime;

        struct Instance
        {
                Runtime::GCPointer<Runtime::Instance> env;
                Runtime::GCPointer<Runtime::Instance> asm2wasm;
                Runtime::GCPointer<Runtime::Instance> global;
                Runtime::GCPointer<Runtime::Instance> keto;

                Runtime::GCPointer<Runtime::MemoryInstance> emscriptenMemory;
        };

        EMSCRIPTEN_API keto::Emscripten::Instance* instantiate(Runtime::Compartment* compartment);
        EMSCRIPTEN_API void initializeGlobals(Runtime::Context* context,const IR::Module& module,Runtime::Instance* moduleInstance);
        EMSCRIPTEN_API void injectCommandArgs(Emscripten::Instance* instance,const std::vector<const char*>& argStrings,std::vector<Runtime::Value>& outInvokeArgs);*/

        struct Thread;
        struct Process;

        bool isEmscriptenModule(const WAVM::IR::Module& irModule);


        std::shared_ptr<Process> createProcess(
                    WAVM::Runtime::Compartment* compartment,
                    std::vector<std::string>&& inArgs,
                    std::vector<std::string>&& inEnvs,
                    WAVM::VFS::VFD* stdIn = nullptr,
                    WAVM::VFS::VFD* stdOut = nullptr,
                    WAVM::VFS::VFD* stdErr = nullptr);

        bool initializeProcess(Process& process,
                               WAVM::Runtime::Context* context,
                               const WAVM::IR::Module& module,
                               WAVM::Runtime::Instance* instance);



        WAVM::Runtime::Resolver& getInstanceResolver(Process& instance);

        void joinAllThreads(Process& instance);

        I32 catchExit(std::function<I32()>&& thunk);

    }
}

#endif
