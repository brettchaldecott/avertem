/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmSessionHttp.hpp
 * Author: ubuntu
 *
 * Created on May 3, 2018, 2:05 PM
 */

#ifndef WAVMHTTPSESSION_HPP
#define WAVMHTTPSESSION_HPP

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


#include "keto/wavm_common/WavmSession.hpp"

namespace keto {
namespace wavm_common {

class WavmSessionHttp;
typedef std::shared_ptr<WavmSessionHttp> WavmSessionHttpPtr;

class WavmSessionHttp : public WavmSession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    WavmSessionHttp(const keto::proto::HttpRequestMessage& httpRequestMessage,
                const keto::crypto::KeyLoaderPtr& keyLoaderPtr);
    WavmSessionHttp(const WavmSessionHttp& orig) = delete;
    virtual ~WavmSessionHttp();

    std::string getSessionType();

    // The http request methods
    std::string getAccount();
    long getNumberOfRoles();
    std::string getRole(long index);
    std::string getTargetUri();
    std::string getMethod();
    std::string getQuery();
    std::string getBody();
    long getNumberOfParameters();
    std::string getParameterKey(long index);
    std::string getParameter(const std::string& key);

    // the http response methods
    void setStatus(long statusCode);
    void setContentType(const std::string& content);
    void setBody(const std::string& body);

    // session queries
    long executeQuery(const std::string& type, const std::string& query);

    // returns the resulting http response message
    keto::proto::HttpResponseMessage getHttpResponse();

private:
    keto::proto::HttpRequestMessage httpRequestMessage;
    keto::proto::HttpResponseMessage httpResponseMessage;
    keto::crypto::KeyLoaderPtr keyLoaderPtr;
    std::map<std::string,std::string> parameters;


};


}
}



#endif /* WAVMSESSION_HPP */

