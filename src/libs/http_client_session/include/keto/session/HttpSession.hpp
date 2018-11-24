/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpSession.hpp
 * Author: Brett Chaldecott
 *
 * Created on February 13, 2018, 7:55 AM
 */

#ifndef HTTP_CLIENT_HTTPSESSION_HPP
#define HTTP_CLIENT_HTTPSESSION_HPP

// std includes
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

// protobuf includes
#include "HandShake.pb.h"


// boost and beast includes
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

// keto includes
#include "keto/common/MetaInfo.hpp"
#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"
#include "keto/ssl/RootCertificate.hpp"
#include "keto/crypto/KeyLoader.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace session {


class HttpSession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    HttpSession(
            boost::asio::io_context& ioc, boost::asio::ssl::context& ctx,
            const std::string& privateKey, const std::string& publicKey);
    HttpSession(const HttpSession& orig) = delete;
    virtual ~HttpSession();
    
    HttpSession& setHost(const std::string& host);
    std::string getHost();
    HttpSession& setPort(const std::string& port);
    std::string getPort();
    
    
    HttpSession& handShake();
    
    std::string makeRequest(keto::transaction_common::TransactionMessageHelperPtr& request);
    
    std::string getSessionId();
    
private:
    boost::asio::io_context& ioc;
    boost::asio::ssl::context& ctx;
    std::string host;
    std::string port;
    keto::crypto::KeyLoader keyLoader;
    bool hasSession;
    keto::proto::ClientResponse clientResponse;
    
    boost::beast::http::request<boost::beast::http::string_body>
    createProtobufRequest(const std::string& endPoint, const std::string& buffer);
    
    boost::beast::http::response<boost::beast::http::string_body> 
    makeRequest(boost::beast::http::request<boost::beast::http::string_body> request);
};


}
}

#endif /* HTTPSESSION_HPP */

