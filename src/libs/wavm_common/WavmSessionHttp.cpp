/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WavmSessionHttp.cpp
 * Author: ubuntu
 * 
 * Created on May 3, 2018, 2:05 PM
 */

#include <cstdlib>
#include <sstream>
#include <math.h>
#include <keto/server_common/Events.hpp>
#include <keto/wavm_common/Constants.hpp>

#include "RDFChange.h"

#include "keto/environment/Units.hpp"
#include "keto/asn1/StatusUtils.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/ServerInfo.hpp"

#include "keto/wavm_common/WavmSessionHttp.hpp"
#include "keto/wavm_common/RDFURLUtils.hpp"
#include "keto/wavm_common/RDFConstants.hpp"
#include "keto/wavm_common/Exception.hpp"
#include "keto/account_query/AccountSparqlQueryHelper.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/transaction_common/TransactionWrapperHelper.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/ChangeSetBuilder.hpp"
#include "keto/transaction_common/SignedChangeSetBuilder.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

namespace keto {
namespace wavm_common {

std::string WavmSessionHttp::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


WavmSessionHttp::WavmSessionHttp(const keto::proto::HttpRequestMessage& httpRequestMessage,
        const keto::crypto::KeyLoaderPtr& keyLoaderPtr) : httpRequestMessage(httpRequestMessage), keyLoaderPtr(keyLoaderPtr) {
    for (int index = 0; index < httpRequestMessage.request_properties_size(); index++) {
        parameters[httpRequestMessage.request_properties(index).key()] =
                httpRequestMessage.request_properties(index).value();
    }
}


WavmSessionHttp::~WavmSessionHttp() {
    
}

std::string WavmSessionHttp::getSessionType() {
    return Constants::SESSION_TYPES::HTTP;
}

// the contract facade methods
std::string WavmSessionHttp::getAccount() {
    keto::asn1::HashHelper hashHelper(httpRequestMessage.account_hash());
    return hashHelper.getHash(keto::common::StringEncoding::HEX);
}

long WavmSessionHttp::getNumberOfRoles() {
    return httpRequestMessage.roles_size();
}

std::string WavmSessionHttp::getRole(long index) {
    if (index >= httpRequestMessage.roles_size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::RoleIndexOutOfBoundsException());
    }
    keto::asn1::HashHelper hashHelper(httpRequestMessage.roles(index));
    return hashHelper.getHash(keto::common::StringEncoding::HEX);
}

std::string WavmSessionHttp::getTargetUri() {
    return httpRequestMessage.uri();
}

std::string WavmSessionHttp::getMethod() {
    return httpRequestMessage.method();
}

std::string WavmSessionHttp::getQuery() {
    return httpRequestMessage.query();
}

std::string WavmSessionHttp::getBody() {
    return httpRequestMessage.body();
}

long WavmSessionHttp::getNumberOfParameters() {
    return httpRequestMessage.request_properties_size();
}

std::string WavmSessionHttp::getParameterKey(long index) {
    if (index >= httpRequestMessage.roles_size()) {
        BOOST_THROW_EXCEPTION(keto::wavm_common::RoleIndexOutOfBoundsException());
    }
    return httpRequestMessage.request_properties(index).key();
}

std::string WavmSessionHttp::getParameter(const std::string& key) {
    if (!parameters.count(key)) {
        return "";
    }
    return parameters[key];
}

// the http response methods
void WavmSessionHttp::setStatus(long statusCode) {
    this->httpResponseMessage.set_status(statusCode);
}

void WavmSessionHttp::setContentType(const std::string& content) {
    this->httpResponseMessage.set_content_type(content);
}

void WavmSessionHttp::setBody(const std::string& body) {
    this->httpResponseMessage.set_body(body);
    this->httpResponseMessage.set_content_length(body.size());
}

long WavmSessionHttp::executeQuery(const std::string& type, const std::string& query) {
    return addResultVectorMap(keto::account_query::AccountSparqlQueryHelper(keto::server_common::Events::SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
                                                       httpRequestMessage.account_hash(),query).execute());
}


keto::proto::HttpResponseMessage WavmSessionHttp::getHttpResponse() {
    return this->httpResponseMessage;
}

}
}
