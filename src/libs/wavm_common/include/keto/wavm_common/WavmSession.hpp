/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmSession.hpp
 * Author: ubuntu
 *
 * Created on May 3, 2018, 2:05 PM
 */

#ifndef WAVMSESSION_HPP
#define WAVMSESSION_HPP

#include <memory>
#include <string>
#include <cstdlib>

#include "Sandbox.pb.h"

#include "keto/asn1/ChangeSetHelper.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/asn1/RDFModelHelper.hpp"
#include "keto/asn1/RDFObjectHelper.hpp"
#include "keto/asn1/NumberHelper.hpp"
#include "keto/wavm_common/RDFMemorySession.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"
#include "keto/crypto/KeyLoader.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace wavm_common {

class WavmSession;
typedef std::shared_ptr<WavmSession> WavmSessionPtr;

class WavmSession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    WavmSession();
    WavmSession(const WavmSession& orig) = delete;
    virtual ~WavmSession();

    virtual std::string getSessionType() = 0;

    virtual std::string getAccount() = 0;

    // session queries
    virtual long executeQuery(const std::string& type, const std::string& query) = 0;
    long getQueryHeaderCount(long id);
    std::string getQueryHeader(long id, long headerNumber);
    std::string getQueryStringValue(long id, long row, long headerNumber);
    std::string getQueryStringValue(long id, long row, const std::string& header);
    long getQueryLongValue(long id, long row, long headerNumber);
    long getQueryLongValue(long id, long row, const std::string& header);
    float getQueryFloatValue(long id, long row, long headerNumber);
    float getQueryFloatValue(long id, long row, const std::string& header);
    long getRowCount(long id);

    // duration
    std::chrono::milliseconds getExecutionTime();

protected:
    std::vector<std::string> getKeys(ResultVectorMap& resultVectorMap);
    long addResultVectorMap(const ResultVectorMap& resultVectorMap);

private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::vector<ResultVectorMap> queryResults;
    
    


};


}
}



#endif /* WAVMSESSION_HPP */

