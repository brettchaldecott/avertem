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

#include "keto/obfuscate/MetaString.hpp"


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
        struct Instance;

        std::shared_ptr<Instance> instantiate(WAVM::Runtime::Compartment* compartment,
        const WAVM::IR::Module& module,
                WAVM::VFS::VFD* stdIn = nullptr,
                WAVM::VFS::VFD* stdOut = nullptr,
                WAVM::VFS::VFD* stdErr = nullptr);
        void initializeGlobals(const std::shared_ptr<Instance>& instance,
                               WAVM::Runtime::Context* context,
                                        const WAVM::IR::Module& module,
                               WAVM::Runtime::Instance* moduleInstance);
        std::vector<WAVM::IR::Value> injectCommandArgs(const std::shared_ptr<Instance>& instance,
                                                          const std::vector<std::string>& argStrings);

        WAVM::Runtime::Resolver& getInstanceResolver(const std::shared_ptr<Instance>& instance);

        void joinAllThreads(Instance* instance);

        I32 catchExit(std::function<I32()>&& thunk);

    }
}

#endif
