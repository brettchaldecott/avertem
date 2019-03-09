/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpRequestManager.cpp
 * Author: ubuntu
 * 
 * Created on February 15, 2018, 10:05 AM
 */

#include <string.h>
#include <iostream>
#include <memory>
#include <keto/common/HttpEndPoints.hpp>
#include "keto/server_session/HttpRequestManager.hpp"
#include "keto/server_session/HttpTransactionManager.hpp"

namespace keto {
namespace server_session {

static std::shared_ptr<HttpRequestManager> singleton;

std::string HttpRequestManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

HttpRequestManager::HttpRequestManager() {
    httpSessionManagerPtr = std::make_shared<HttpSessionManager>();
    httpTransactionManagerPtr = std::make_shared<HttpTransactionManager>(
            httpSessionManagerPtr);
    this->httpSparqlManagerPtr = std::make_shared<HttpSparqlManager>(
            httpSessionManagerPtr);
    this->httpContractManagerPtr = std::make_shared<HttpContractManager>(
            httpSessionManagerPtr);
}

HttpRequestManager::~HttpRequestManager() {
    httpSparqlManagerPtr.reset();
    httpTransactionManagerPtr.reset();
    httpSessionManagerPtr.reset();
    
}

bool
HttpRequestManager::checkRequest(boost::beast::http::request<boost::beast::http::string_body>& req) {
    boost::beast::string_view path = req.target();
    if (path.empty()) {
        return false;
    }
    std::string target = path.to_string();
    if (0 == target.compare(keto::common::HttpEndPoints::HAND_SHAKE)) {
        return true;
    } else if (0 == target.compare(keto::common::HttpEndPoints::TRANSACTION)) {
        return true;
    } else if (strlen(keto::common::HttpEndPoints::DATA_QUERY) <= target.size() &&
        0 == target.compare(0,strlen(keto::common::HttpEndPoints::DATA_QUERY),keto::common::HttpEndPoints::DATA_QUERY)) {
        return true;
    } else if (strlen(keto::common::HttpEndPoints::CONTRACT) <= target.size() &&
        0 == target.compare(0,strlen(keto::common::HttpEndPoints::CONTRACT),keto::common::HttpEndPoints::CONTRACT)) {
        return true;
    } else if (strlen(keto::common::HttpEndPoints::AUTHENTICATE) <= target.size() &&
        0 == target.compare(0,strlen(keto::common::HttpEndPoints::AUTHENTICATE),keto::common::HttpEndPoints::AUTHENTICATE)) {
        return true;
    }
    
    return false;
}

boost::beast::http::response<boost::beast::http::string_body>
HttpRequestManager::handle_request(
        boost::beast::http::request<boost::beast::http::string_body>& req) {
    boost::beast::string_view path = req.target();
    std::string target = path.to_string();
    std::string result;
    if (strlen(keto::common::HttpEndPoints::AUTHENTICATE) <= target.size() &&
        0 == target.compare(0,strlen(keto::common::HttpEndPoints::AUTHENTICATE),keto::common::HttpEndPoints::AUTHENTICATE)) {
        result = this->httpSessionManagerPtr->processHello(req.body());
    } else if (0 == target.compare(keto::common::HttpEndPoints::HAND_SHAKE)) {
        result = this->httpSessionManagerPtr->processHello(req.body());
    } else if (0 == target.compare(keto::common::HttpEndPoints::TRANSACTION)) {
        result = this->httpTransactionManagerPtr->processTransaction(req,req.body());
    } else if (strlen(keto::common::HttpEndPoints::DATA_QUERY) <= target.size() &&
               0 == target.compare(0,strlen(keto::common::HttpEndPoints::DATA_QUERY),keto::common::HttpEndPoints::DATA_QUERY)) {
        result = this->httpSparqlManagerPtr->processQuery(req,req.body());
        // for the sparql queries we force the connect to close otherwise
        // the body gets appended to rather than freshly executed.
        boost::beast::http::response<boost::beast::http::string_body> response;
        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::field::content_type, "application/sparql-results+json");
        response.keep_alive(false);
        response.chunked(false);
        response.body() = result;
        response.content_length(result.size());
        return response;
    } else if (strlen(keto::common::HttpEndPoints::CONTRACT) <= target.size() &&
               0 == target.compare(0,strlen(keto::common::HttpEndPoints::CONTRACT),keto::common::HttpEndPoints::CONTRACT)) {
        return this->httpContractManagerPtr->processQuery(req,req.body());
    }
    boost::beast::http::response<boost::beast::http::string_body> response;
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/html");
    response.keep_alive(req.keep_alive());
    response.body() = result;
    response.content_length(result.size());
    
    return response;
}

std::shared_ptr<HttpRequestManager> HttpRequestManager::init() {
    if (!singleton) {
        singleton = std::shared_ptr<HttpRequestManager>(
                new HttpRequestManager());
    }
    return singleton;
}

void HttpRequestManager::fin() {
    singleton.reset();
}


std::shared_ptr<HttpRequestManager> HttpRequestManager::getInstance() {
    return singleton;
}



}
}

