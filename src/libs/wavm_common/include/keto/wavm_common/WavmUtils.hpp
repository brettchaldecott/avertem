/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmUtils.hpp
 * Author: ubuntu
 *
 * Created on April 22, 2018, 5:42 PM
 */

#ifndef WAVMUTILS_HPP
#define WAVMUTILS_HPP

#include <string>
#include <vector>

#include "Inline/BasicTypes.h"
#include "Runtime/Runtime.h"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace wavm_common {

class WavmUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    WavmUtils() = delete;
    WavmUtils(const WavmUtils& orig) = delete;
    virtual ~WavmUtils() = delete;
    
    static std::string readCString(Runtime::MemoryInstance* memory,I32 stringAddress);
    static std::string readTypeScriptString(Runtime::MemoryInstance* memory,I32 stringAddress);
    static std::vector<char> buildTypeScriptString(const std::string& string);
    static void log(uint32_t intLevel,const std::string& msg);
private:

};

}
}


#endif /* WAVMUTILS_HPP */

