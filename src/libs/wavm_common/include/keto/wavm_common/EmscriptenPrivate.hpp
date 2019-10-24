//
// Created by Brett Chaldecott on 2019-10-15.
//

#ifndef KETO_EMSCRIPTENPRIVATE_HPP
#define KETO_EMSCRIPTENPRIVATE_HPP

namespace WAVM {
    namespace Emscripten {

        namespace emabi {
            typedef U32 Address;
            static constexpr Address addressMin = 0;
            static constexpr Address addressMax = UINT32_MAX;

            typedef U32 Size;
            static constexpr Size sizeMin = 0;
            static constexpr Size sizeMax = UINT32_MAX;

            typedef I32 Int;
            static constexpr Int intMin = INT32_MIN;
            static constexpr Int intMax = INT32_MAX;

            typedef I32 Long;
            static constexpr Long longMin = INT32_MIN;
            static constexpr Long longMax = INT32_MAX;

            typedef I32 FD;

            typedef U32 pthread_t;
            typedef U32 pthread_key_t;

            typedef Int Result;
            static constexpr Int resultMin = intMin;
            static constexpr Int resultMax = intMax;
        }


        enum class ioStreamVMHandle
        {
            StdIn = 0,
            StdOut = 1,
            StdErr = 2,
        };

        struct ExitException {
            U32 exitCode;
        };


        inline WAVM::Emscripten::emabi::Address coerce32bitAddress(WAVM::Runtime::Memory* memory, Uptr address)
        {
            if(address >= WAVM::Emscripten::emabi::addressMax)
            {
                Runtime::throwException(Runtime::ExceptionTypes::outOfBoundsMemoryAccess,
                                        {Runtime::asObject(memory), U64(address)});
            }
            return (U32)address;
        }

        inline WAVM::Emscripten::emabi::Result coerce32bitAddressResult(Runtime::Memory* memory, Iptr address)
        {
            if(address >= WAVM::Emscripten::emabi::resultMax)
            {
                Runtime::throwException(Runtime::ExceptionTypes::outOfBoundsMemoryAccess,
                                        {Runtime::asObject(memory), U64(address)});
            }
            return (WAVM::Emscripten::emabi::Result)address;
        }
    }
}


#endif //KETO_EMSCRIPTENPRIVATE_HPP
