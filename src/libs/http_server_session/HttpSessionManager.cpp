/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpSessionManager.cpp
 * Author: ubuntu
 * 
 * Created on February 15, 2018, 9:39 AM
 */

#include <iostream>
#include <sstream>
#include <memory>

#include "Sparql.pb.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <condition_variable>

#include <botan/hex.h>


#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/server_session/HttpSessionManager.hpp"

#include "keto/server_common/VectorUtils.hpp"


#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/server_session/Exception.hpp"
#include "keto/server_session/Constants.hpp"
#include "keto/server_session/URIAuthenticationParser.hpp"

#include "keto/server_common/StringUtils.hpp"

#include "keto/account_query/AccountSparqlQueryHelper.hpp"

#include "keto/crypto/KeyLoader.hpp"
#include "keto/crypto/SignatureVerification.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/crypto/HashGenerator.hpp"
#include "keto/crypto/BlockchainPublicKeyLoader.hpp"
#include "keto/event/EventServiceInterface.hpp"

namespace keto {
namespace server_session {

std::string HttpSessionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

HttpSessionManager::HttpSessionManager() {
    // retrieve the configuration
    std::shared_ptr<keto::environment::Config> config = keto::environment::EnvironmentManager::getInstance()->getConfig();

    //if (config->getVariablesMap().count(Constants::HTTP_SESSION_ACCOUNT)) {
    //    sessionAccount = Botan::hex_decode(config->getVariablesMap()[Constants::HTTP_SESSION_ACCOUNT].as<std::string>(),true);
    //} else {
        sessionAccount = keto::server_common::ServerInfo::getInstance()->getAccountHash();
    //}

}


HttpSessionManager::~HttpSessionManager() {
}

boost::beast::http::response<boost::beast::http::string_body> HttpSessionManager::processHello(const std::string& hello) {
    keto::proto::ClientHello clientHello;
    if (!clientHello.ParseFromString(hello)) {
        BOOST_THROW_EXCEPTION(keto::server_session::MessageDeserializationException(
                "Failed to deserialized the Client Hello message."));
    }

    // this method is called to validate the client
    std::shared_ptr<Botan::Public_Key> publicKey = this->validateRemoteHash(clientHello);
    if (!publicKey) {
        std::string result;
        keto::proto::ClientResponse response;
        response.set_response(keto::proto::HelloResponse::GO_AWAY);
        response.SerializeToString(&result);

        return buildResponse(result,403,Constants::CONTENT_TYPE::PROTOBUF);
    }
    keto::server_common::VectorUtils vectorUtils;
    std::vector<uint8_t> clientHash = vectorUtils.copyStringToVector(
        clientHello.client_hash());

    std::shared_ptr<HttpSession> ptr;

    if (this->clientHashMap.count(clientHash)) {
        ptr = this->clientHashMap[clientHash];
    } else {
        ptr = std::shared_ptr<HttpSession>(new HttpSession(clientHash,this->sessionAccount));
        this->clientHashMap[ptr->getClientHash()] = ptr;
        this->clientSessionMap[ptr->getSessionHash()] = ptr;
    }

    std::string result;
    keto::proto::ClientResponse response;
    response.set_response(keto::proto::HelloResponse::WELCOME);
    response.set_session_hash(vectorUtils.copyVectorToString(ptr->getSessionHash()));
    response.set_session_key(vectorUtils.copyVectorToString(
            keto::crypto::SecureVectorUtils().copyFromSecure(
            ptr->getSessionKey())));
    response.SerializeToString(&result);
    return buildResponse(result,200,Constants::CONTENT_TYPE::PROTOBUF);
}


boost::beast::http::response<boost::beast::http::string_body> HttpSessionManager::processAuthentication(
        boost::beast::http::request<boost::beast::http::string_body>& req,
        const std::string& body) {
    if( req.method() == boost::beast::http::verb::options) {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.set(boost::beast::http::field::access_control_allow_origin, "*");
        res.set(boost::beast::http::field::access_control_allow_methods, "*");
        res.set(boost::beast::http::field::access_control_allow_headers, "*");
        res.keep_alive(req.keep_alive());
        res.body() = "";
        res.prepare_payload();
        return res;
    }

    boost::beast::string_view path = req.target();
    std::string target = keto::server_common::StringUtils(path.to_string()).replaceAll("//","/");
    URIAuthenticationParser uriAuthenticationParser(target);

    keto::proto::SparqlResultSetQuery sparqlResultSetQuery;
    sparqlResultSetQuery.set_account_hash(uriAuthenticationParser.getAccountHash());
    std::stringstream ss;
    ss << "SELECT ?publicKey WHERE { " <<
       "?account <http://keto-coin.io/schema/rdf/1.0/keto/Account#hash> '"
       << uriAuthenticationParser.getAccountHash().getHash(keto::common::StringEncoding::HEX)
       << "'^^<http://www.w3.org/2001/XMLSchema#string> . " <<
       "?account <http://keto-coin.io/schema/rdf/1.0/keto/Account#public_key> ?publicKey . } LIMIT 1";

    keto::account_query::ResultVectorMap resultVectorMap =
            keto::account_query::AccountSparqlQueryHelper(keto::server_common::Events::SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
                                                  uriAuthenticationParser.getAccountHash(),ss.str()).execute();
    if (resultVectorMap.size() != 1) {
        KETO_LOG_INFO << "Cannot find the account";
        return buildResponse("Invalid account",403);
    }

    if (!keto::crypto::SignatureVerification(Botan::hex_decode(resultVectorMap[0]["publicKey"]),
            (std::vector<uint8_t>)uriAuthenticationParser.getSourceHash()).check(uriAuthenticationParser.getSignature())) {
        KETO_LOG_INFO << "The signature is invalid [" << uriAuthenticationParser.getSourceHash().getHash(keto::common::HEX) << "][" <<
            Botan::hex_encode(uriAuthenticationParser.getSignature(),true) << "]";
        return buildResponse("Invalid signature",403);
    }

    std::shared_ptr<HttpSession> ptr;

    if (this->clientHashMap.count(uriAuthenticationParser.getSourceHash())) {
        ptr = this->clientHashMap[uriAuthenticationParser.getSourceHash()];
    } else {
        ptr = std::shared_ptr<HttpSession>(new HttpSession(uriAuthenticationParser.getSourceHash(),uriAuthenticationParser.getAccountHash()));
        this->clientHashMap[ptr->getClientHash()] = ptr;
        this->clientSessionMap[ptr->getSessionHash()] = ptr;
    }


    std::stringstream json;
    json << "{\"session\":\"" << Botan::hex_encode(ptr->getSessionHash(),true) << "\"}";
    if (uriAuthenticationParser.isCors()) {
        return buildResponse(json.str(),200,Constants::CONTENT_TYPE::TEXT);
    } else {
        return buildResponse(json.str(),200,Constants::CONTENT_TYPE::JSON);
    }

}


bool HttpSessionManager::isValid(const std::vector<uint8_t>& sessionHash) {
    if (this->clientSessionMap.count(sessionHash)) {
        return true;
    }
    return false;
}

std::shared_ptr<HttpSession> HttpSessionManager::getSession(
    const std::vector<uint8_t>& sessionHash) {
    return this->clientSessionMap[sessionHash];
}


std::shared_ptr<Botan::Public_Key> HttpSessionManager::validateRemoteHash(
    keto::proto::ClientHello& clientHello) {

    // retrieve a list of all the public keys
    boost::filesystem::path publicKeyPath =
            keto::server_common::ServerInfo::getInstance()->getPublicKeyPath();
    std::vector<boost::filesystem::path> files;                                // so we can sort them later
    std::copy(boost::filesystem::directory_iterator(publicKeyPath),
            boost::filesystem::directory_iterator(), std::back_inserter(files));

    std::vector<uint8_t> clientHash = keto::server_common::VectorUtils().copyStringToVector(clientHello.client_hash());
    std::vector<uint8_t> signature = keto::server_common::VectorUtils().copyStringToVector(clientHello.signature());

    for (std::vector<boost::filesystem::path>::const_iterator it(files.begin()),
            it_end(files.end()); it != it_end; ++it) {
        keto::crypto::KeyLoader loader(*it);
        std::shared_ptr<Botan::Public_Key> publicKey = loader.getPublicKey();

        std::vector<uint8_t> publicKeyHashVector = keto::crypto::SecureVectorUtils().copyFromSecure(keto::crypto::HashGenerator().generateHash(
            Botan::X509::BER_encode(*publicKey)));

        if (publicKeyHashVector == clientHash) {
            KETO_LOG_DEBUG << "[HttpSessionManager::validateRemoteHash] Check the signature [" <<
                           Botan::hex_encode(publicKeyHashVector,true) << "][" << Botan::hex_encode(clientHash,true) << "]";
            if (keto::crypto::SignatureVerification(publicKey,publicKeyHashVector).check(signature)) {
                return publicKey;
            } else {
                break;
            }
        }
    }
    KETO_LOG_ERROR << "[HttpSessionManager::validateRemoteHash] Failed to vaidate the hash for [" << Botan::hex_encode(clientHash,true)
        << "][" << Botan::hex_encode(signature,true) << "]";
    return std::shared_ptr<Botan::Public_Key>();
}


boost::beast::http::response<boost::beast::http::string_body> HttpSessionManager::buildResponse(
        const std::string body, int status, const std::string& contentType) {
    boost::beast::http::response<boost::beast::http::string_body> response;
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, contentType);
    //response.set(boost::beast::http::field::access_control_request_headers, "*");
    response.set(boost::beast::http::field::access_control_allow_origin, "*");
    response.result(boost::beast::http::int_to_status(status));
    response.keep_alive(false);
    response.chunked(false);
    response.body() = body;
    response.content_length(body.size());
    return response;
}

}
}
