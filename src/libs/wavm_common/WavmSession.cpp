/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmSession.cpp
 * Author: ubuntu
 * 
 * Created on May 3, 2018, 2:05 PM
 */

#include <cstdlib>
#include <sstream>
#include <math.h>

#include "RDFChange.h"

#include "keto/environment/Units.hpp"
#include "keto/asn1/StatusUtils.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"

#include "keto/wavm_common/WavmSession.hpp"
#include "keto/wavm_common/RDFURLUtils.hpp"
#include "keto/wavm_common/RDFConstants.hpp"
#include "keto/wavm_common/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/ChangeSetBuilder.hpp"
#include "keto/transaction_common/SignedChangeSetBuilder.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

namespace keto {
namespace wavm_common {

std::string WavmSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
WavmSession::WavmSession() {
    this->startTime = std::chrono::high_resolution_clock::now();
}

WavmSession::~WavmSession() {
    
}


long WavmSession::getQueryHeaderCount(long id) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    return getKeys(resultVectorMap).size();
}

std::string WavmSession::getQueryHeader(long id, long headerNumber) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    std::vector<std::string> keys = getKeys(resultVectorMap);
    if (headerNumber >= keys.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << headerNumber << "] currently have [" << keys.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    return keys[headerNumber];
}

std::string WavmSession::getQueryStringValue(long id, long row, long headerNumber) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    if (row >= resultVectorMap.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << row << "] currently have [" << resultVectorMap.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    return resultVectorMap[row][getKeys(resultVectorMap)[headerNumber]];
}

std::string WavmSession::getQueryStringValue(long id, long row, const std::string& header) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    if (row >= resultVectorMap.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << row << "] currently have [" << resultVectorMap.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    return resultVectorMap[row][header];
}

long WavmSession::getQueryLongValue(long id, long row, long headerNumber) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    if (row >= resultVectorMap.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << row << "] currently have [" << resultVectorMap.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    return std::stol(resultVectorMap[row][getKeys(resultVectorMap)[headerNumber]]);
}

long WavmSession::getQueryLongValue(long id, long row, const std::string& header) {
    //KETO_LOG_DEBUG << "[WavmSession::getQueryLongValue]The query long value [" << id << "]";
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    //KETO_LOG_DEBUG << "[WavmSession::getQueryLongValue]The row [" << row<< "]";
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    if (row >= resultVectorMap.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << row << "] currently have [" << resultVectorMap.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    //KETO_LOG_DEBUG << "[WavmSession::getQueryLongValue]The header [" << header << "]";
    return std::stol(resultVectorMap[row][header]);
}

float WavmSession::getQueryFloatValue(long id, long row, long headerNumber) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    if (row >= resultVectorMap.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << row << "] currently have [" << resultVectorMap.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    return std::stol(resultVectorMap[row][getKeys(resultVectorMap)[headerNumber]]);
}

float WavmSession::getQueryFloatValue(long id, long row, const std::string& header) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    if (row >= resultVectorMap.size()) {
        std::stringstream ss;
        ss << "The requested row is out of range [" << row << "] currently have [" << resultVectorMap.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    return std::stol(resultVectorMap[row][header]);
}

long WavmSession::getRowCount(long id) {
    if (id >= this->queryResults.size()) {
        std::stringstream ss;
        ss << "The requested index is out of range [" << id << "] currently have [" << this->queryResults.size() << "]";
        BOOST_THROW_EXCEPTION(keto::wavm_common::RdfIndexOutOfRangeException(ss.str()));
    }
    ResultVectorMap& resultVectorMap = this->queryResults[id];
    return resultVectorMap.size();
}

std::chrono::milliseconds WavmSession::getExecutionTime() {
    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
}

std::vector<std::string> WavmSession::getKeys(ResultVectorMap& resultVectorMap) {
    std::vector<std::string> keys;
    if (!resultVectorMap.size()) {
        return std::vector<std::string>();
    }
    ResultMap& resultMap = resultVectorMap[0];
    for(std::map<std::string,std::string>::iterator it = resultMap.begin(); it != resultMap.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

long WavmSession::addResultVectorMap(const ResultVectorMap& resultVectorMap) {
    queryResults.push_back(resultVectorMap);
    return queryResults.size() - 1;
}


}
}
