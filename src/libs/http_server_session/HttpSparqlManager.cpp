/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpSparqlManager.cpp
 * Author: ubuntu
 * 
 * Created on March 28, 2018, 11:02 AM
 */

#include <string>
#include <iostream>
#include <boost/beast/http/message.hpp>

#include "Sparql.pb.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/common/HttpEndPoints.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/server_session/Exception.hpp"
#include "keto/server_session/HttpSparqlManager.hpp"
#include "keto/server_session/URISparqlParser.hpp"

namespace keto{
namespace server_session {

std::string HttpSparqlManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
HttpSparqlManager::HttpSparqlManager(std::shared_ptr<HttpSessionManager>& httpSessionManagerPtr) : 
    httpSessionManagerPtr(httpSessionManagerPtr) {
    
}

HttpSparqlManager::~HttpSparqlManager() {
    
}

std::string HttpSparqlManager::processQuery(
        boost::beast::http::request<boost::beast::http::string_body>& req,
        const std::string& body) {
    if (!req.base().count(keto::common::HttpEndPoints::HEADER_SESSION_HASH)) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }
    std::string sessionHash = (const std::string&)req.base().at(keto::common::HttpEndPoints::HEADER_SESSION_HASH);
    keto::asn1::HashHelper sessionHashHelper(
            sessionHash,keto::common::HEX);
    std::vector<uint8_t> vectorHash = keto::crypto::SecureVectorUtils().copyFromSecure(sessionHashHelper);
    if (!httpSessionManagerPtr->isValid(vectorHash)) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }

    std::shared_ptr<HttpSession> httpSession = httpSessionManagerPtr->getSession(vectorHash);

    boost::beast::string_view path = req.target();
    std::string target = keto::server_common::StringUtils(path.to_string()).replaceAll("//","/");
    URISparqlParser uriSparql(target,body);
    
    //keto::asn1::HashHelper hashHelper(uriSparql.getAccountHash(),keto::common::HEX);
    
    keto::proto::SparqlQuery sparqlQuery;
    sparqlQuery.set_account_hash(keto::asn1::HashHelper(httpSession->getAccountHash()));
    sparqlQuery.set_query(uriSparql.getQuery());
    
    sparqlQuery = 
            keto::server_common::fromEvent<keto::proto::SparqlQuery>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SparqlQuery>(
            keto::server_common::Events::SPARQL_QUERY_MESSAGE,sparqlQuery)));
    
    return sparqlQuery.result();
}

}
}