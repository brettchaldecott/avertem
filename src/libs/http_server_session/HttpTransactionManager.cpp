/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpTransactionManager.cpp
 * Author: ubuntu
 * 
 * Created on March 2, 2018, 7:11 AM
 */

#include <sstream>

#include <boost/beast/http/message.hpp>

#include <botan/hex.h>

#include "BlockChain.pb.h"
#include "Protocol.pb.h"
#include "Sparql.pb.h"

#include "google/protobuf/any.h"

#include "keto/common/HttpEndPoints.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/asn1/HashHelper.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/key_store_utils/Events.hpp"

#include "keto/server_session/HttpTransactionManager.hpp"
#include "keto/server_session/HttpSessionManager.hpp"
#include "keto/server_session/HttpSession.hpp"
#include "keto/server_session/Exception.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/crypto/SignatureVerification.hpp"

#include "keto/server_common/StringUtils.hpp"

#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/account_query/AccountSparqlQueryHelper.hpp"

namespace keto {
namespace server_session {

std::string HttpTransactionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

HttpTransactionManager::HttpTransactionManager(
    std::shared_ptr<HttpSessionManager>& httpSessionManagerPtr) : 
    httpSessionManagerPtr(httpSessionManagerPtr) {
    
}

HttpTransactionManager::~HttpTransactionManager() {
}

std::string HttpTransactionManager::    processTransaction(
        boost::beast::http::request<boost::beast::http::string_body>& req,
        const std::string& transactionMsg) {
    boost::beast::string_view path = req.target();
    std::string target = keto::server_common::StringUtils(path.to_string()).replaceAll("//","/");
    std::string subUri = target.substr(strlen(keto::common::HttpEndPoints::TRANSACTION));

    std::string sessionHash;
    if (req.base().count(keto::common::HttpEndPoints::HEADER_SESSION_HASH)) {
        sessionHash = (const std::string &) req.base().at(keto::common::HttpEndPoints::HEADER_SESSION_HASH);
    } else {
        int nextSlash = subUri.find("/");
        if (nextSlash == std::string::npos) {
            BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
        }
        sessionHash = subUri.substr(nextSlash+1);
    }
    keto::asn1::HashHelper hashHelper(
            sessionHash,keto::common::HEX);
    std::vector<uint8_t> vectorHash = keto::crypto::SecureVectorUtils().copyFromSecure(hashHelper);
    if (!httpSessionManagerPtr->isValid(vectorHash)) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }
    std::shared_ptr<HttpSession> httpSession = 
            httpSessionManagerPtr->getSession(vectorHash);
    
    keto::proto::Transaction transaction;
    
    if (!transaction.ParseFromString(transactionMsg)) {
        BOOST_THROW_EXCEPTION(keto::server_session::MessageDeserializationException(
                "Failed to deserialized the transaction message."));
    }
    
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper;
    messageWrapperProtoHelper.setSessionHash(hashHelper).setTransaction(transaction);

    // validate the signature
    if (!validateSignature(
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper()->getTransactionWrapper()->getHash(),
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper()->getTransactionWrapper()->getSignature(),
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper()->getTransactionWrapper()->getCurrentAccount())) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidAccountSigner());
    }
    
    //KETO_LOG_DEBUG << "Before re-encrypting the transaction";
    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    messageWrapper = keto::server_common::fromEvent<keto::proto::MessageWrapper>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                keto::key_store_utils::Events::TRANSACTION::REENCRYPT_TRANSACTION,messageWrapper)));
    
    //KETO_LOG_DEBUG << "After re-encryhpting the transaction";
    keto::proto::MessageWrapperResponse  messageWrapperResponse = 
            keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
            keto::server_common::Events::ROUTE_MESSAGE,messageWrapper)));
    
    return messageWrapperResponse.result();
}


bool HttpTransactionManager::validateSignature(
        const keto::asn1::HashHelper& transactionHash,
        const keto::asn1::SignatureHelper&  transactionSignature,
        const keto::asn1::HashHelper& accountHash) {

    keto::proto::SparqlResultSetQuery sparqlResultSetQuery;
    sparqlResultSetQuery.set_account_hash(accountHash);
    std::stringstream ss;
    ss << "SELECT ?publicKey WHERE { " <<
       "?account <http://keto-coin.io/schema/rdf/1.0/keto/Account#hash> '"
       << accountHash.getHash(keto::common::StringEncoding::HEX)
       << "'^^<http://www.w3.org/2001/XMLSchema#string> . " <<
       "?account <http://keto-coin.io/schema/rdf/1.0/keto/Account#public_key> ?publicKey . } LIMIT 1";

    keto::account_query::ResultVectorMap resultVectorMap =
            keto::account_query::AccountSparqlQueryHelper(keto::server_common::Events::SPARQL_QUERY_WITH_RESULTSET_MESSAGE,
                                                          accountHash,ss.str()).execute();
    if (resultVectorMap.size() != 1) {
        KETO_LOG_INFO << "Cannot find the account";
        return false;
    }

    KETO_LOG_ERROR << "Load the public key : " << resultVectorMap[0]["publicKey"];
    if (!keto::crypto::SignatureVerification(Botan::hex_decode(resultVectorMap[0]["publicKey"]),
                                             (std::vector<uint8_t>)transactionHash).check(transactionSignature)) {
        KETO_LOG_ERROR << "The signature is invalid [" << transactionHash.getHash(keto::common::HEX) << "][" <<
                      Botan::hex_encode((std::vector<uint8_t>)transactionSignature,true) << "]";
        return false;
    }
    return true;
}
    
}
}
