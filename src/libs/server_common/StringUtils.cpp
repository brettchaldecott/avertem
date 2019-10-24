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
    if (!value.size()) {
        return result;
    }
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

std::string StringUtils::replaceAll(const std::string& oldVal, const std::string& newVal) {
    std::string result = this->value;
    int pos = 0;
    int begin = 0;
    while ( (begin = result.find(oldVal,pos)) != result.npos ) {
        result.replace(begin,oldVal.size(),newVal);
        pos = begin;
    }
    return result;
}

bool StringUtils::isIEqual(const std::string& val) {
    std::string value = val;
    return ((this->value.size() == value.size()) && std::equal(this->value.begin(), this->value.end(), value.begin(), [](char & c1, char & c2){
        return (c1 == c2 || std::toupper(c1) == std::toupper(c2));
    }));
}

}
}
