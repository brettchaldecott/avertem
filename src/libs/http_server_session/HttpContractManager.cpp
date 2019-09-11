//
// Created by Brett Chaldecott on 2019/03/06.
//

#include "keto/server_session/HttpContractManager.hpp"


#include <string>
#include <iostream>
#include <boost/beast/http/message.hpp>

#include "keto/asn1/HashHelper.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/common/HttpEndPoints.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"


#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/server_session/Exception.hpp"
#include "keto/server_session/HttpContractManager.hpp"

#include "include/keto/server_session/URISparqlParser.hpp"
#include "keto/server_session/URIContractParser.hpp"

namespace keto{
namespace server_session {

std::string HttpContractManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

HttpContractManager::HttpContractManager(std::shared_ptr<HttpSessionManager>& httpSessionManagerPtr) :
        httpSessionManagerPtr(httpSessionManagerPtr) {

}

HttpContractManager::~HttpContractManager() {

}

boost::beast::http::response<boost::beast::http::string_body> HttpContractManager::processQuery(
        boost::beast::http::request<boost::beast::http::string_body>& req,
        const std::string& body) {
    boost::beast::string_view path = req.target();
    std::string target = path.to_string();
    URIContractParser uriContract(target);

    std::string sessionHash;
    if (req.base().count(keto::common::HttpEndPoints::HEADER_SESSION_HASH)) {
        sessionHash = (const std::string&)req.base().at(keto::common::HttpEndPoints::HEADER_SESSION_HASH);
    } else if (uriContract.hasSessionHash()) {
        sessionHash = uriContract.getSessionHash();
    } else {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }

    keto::asn1::HashHelper sessionHashHelper(
            sessionHash,keto::common::HEX);
    std::vector<uint8_t> vectorHash = keto::crypto::SecureVectorUtils().copyFromSecure(sessionHashHelper);
    if (!httpSessionManagerPtr->isValid(vectorHash)) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }

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

    std::shared_ptr<HttpSession> httpSession = httpSessionManagerPtr->getSession(vectorHash);

    std::string contract =
            getContractByHash(httpSession->getAccountHash(),
                    keto::asn1::HashHelper(uriContract.getContractHash(),keto::common::HEX));

    keto::proto::HttpRequestMessage httpRequestMessage;
    httpRequestMessage.set_contract(contract);
    httpRequestMessage.set_account_hash(keto::server_common::VectorUtils().copyVectorToString(
            httpSession->getAccountHash()));
    for (std::vector<uint8_t> role : httpSession->getRoles()) {
        *httpRequestMessage.add_roles() = keto::server_common::VectorUtils().copyVectorToString(
                role);
    }
    httpRequestMessage.set_method(req.method_string().to_string());
    httpRequestMessage.set_body(req.body());
    httpRequestMessage.set_uri(uriContract.getRequestUri());
    httpRequestMessage.set_query(uriContract.getQuery());
    copyHttpParameters(uriContract.getParameters(),httpRequestMessage);

    keto::proto::HttpResponseMessage httpResponseMessage = keto::server_common::fromEvent<keto::proto::HttpResponseMessage>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::HttpRequestMessage>(
                    keto::server_common::Events::EXECUTE_HTTP_CONTRACT_MESSAGE,httpRequestMessage)));

    boost::beast::http::response<boost::beast::http::string_body> response;
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    if (uriContract.isCors()) {
        response.set(boost::beast::http::field::content_type, Constants::CONTENT_TYPE::TEXT);
    } else {
        response.set(boost::beast::http::field::content_type, httpResponseMessage.content_type());
    }
    //response.set(boost::beast::http::field::access_control_request_headers, "*");
    response.set(boost::beast::http::field::access_control_allow_origin, "*");
    response.result(boost::beast::http::int_to_status(httpResponseMessage.status()));
    response.keep_alive(false);
    response.chunked(false);
    KETO_LOG_DEBUG << "The content : " << httpResponseMessage.body();
    response.body() = httpResponseMessage.body();
    response.content_length(httpResponseMessage.content_length());
    return response;
}


std::string HttpContractManager::getContractByHash(const keto::asn1::HashHelper& account, const std::string& hash) {
    keto::proto::ContractMessage contractMessage;
    contractMessage.set_account_hash(account);
    contractMessage.set_contract_hash(hash);
    return getContract(contractMessage).contract();
}

keto::proto::ContractMessage HttpContractManager::getContract(keto::proto::ContractMessage& contractMessage) {
    return keto::server_common::fromEvent<keto::proto::ContractMessage>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::ContractMessage>(
                    keto::server_common::Events::GET_CONTRACT,contractMessage)));
}


void HttpContractManager::copyHttpParameters(
        std::map<std::string,std::string> parameters,
        keto::proto::HttpRequestMessage& httpRequestMessage) {

    for (auto const& column : parameters) {
        keto::proto::HttpRequestEntry entry;
        entry.set_key(column.first);
        entry.set_value(column.second);
        *httpRequestMessage.add_request_properties() = entry;
    }
}

}
}
