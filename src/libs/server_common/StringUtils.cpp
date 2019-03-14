/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StringUtils.cpp
 * Author: ubuntu
 * 
 * Created on May 24, 2018, 8:12 AM
 */

#include <string>
#include <vector>

#include "keto/server_common/StringUtils.hpp"

namespace keto {
namespace server_common {

std::string StringUtils::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

StringUtils::StringUtils(const std::string& value) : value(value) {
}

StringUtils::~StringUtils() {
}

StringVector StringUtils::tokenize(const std::string& token) {
    StringVector result;
    int start = 0;
    int end = value.find(token);
    while (end != std::string::npos)
    {   
        result.push_back(value.substr(start,(end - start)));
        start = end + token.length();
        end = value.find(token, start);
    }
    result.push_back(value.substr(start,(end - start)));
    
    return result;
}

bool StringUtils::isHexidecimal(const std::string& value) {
    for (char character : value) {
        if (!isxdigit(character)) {
            return false;
        }
    }
    return true;
}

}
}