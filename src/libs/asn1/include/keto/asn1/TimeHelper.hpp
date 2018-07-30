/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimeHelper.hpp
 * Author: ubuntu
 *
 * Created on January 31, 2018, 8:12 AM
 */

#ifndef TIMEHELPER_HPP
#define TIMEHELPER_HPP

#include "UTCTime.h"
#include <chrono>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace asn1 {

class TimeHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();

    
    TimeHelper();
    TimeHelper(const UTCTime_t& time);
    TimeHelper(const TimeHelper& orig) = default;
    virtual ~TimeHelper();
    
    
    TimeHelper& operator =(const UTCTime_t& time);
    TimeHelper& operator =(const UTCTime_t* time);
    /**
     * The internal memory is assumed to be owned by the last person in the row
     * and thus must be freed by that code.
     * 
     * @return A utc time copy.
     */
    operator UTCTime_t() const;
    TimeHelper& operator =(const std::time_t& time);
    operator std::time_t() const;
    TimeHelper& operator =(const std::chrono::system_clock::time_point& time);
    operator std::chrono::system_clock::time_point() const;
    
private:
    std::chrono::system_clock::time_point time_point;
};

}
}

#endif /* TIMEHELPER_HPP */

