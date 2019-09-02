/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StringUtils.hpp
 * Author: ubuntu
 *
 * Created on May 24, 2018, 8:12 AM
 */

#ifndef KETO_STRINGUTILS_HPP
#define KETO_STRINGUTILS_HPP

#include <string>
#include <vector>
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace server_common {
    
typedef std::vector<std::string> StringVector;

class StringUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    StringUtils(const std::string& value);
    StringUtils(const StringUtils& orig) = default;
    virtual ~StringUtils();
    
    StringVector tokenize(const std::string& token);
    static bool isHexidecimal(const std::string& value);
private:
    std::string value;
};


}
}


#endif /* STRINGUTILS_HPP */

