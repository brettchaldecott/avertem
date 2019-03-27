/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpSessionManager.hpp
 * Author: ubuntu
 *
 * Created on February 15, 2018, 9:39 AM
 */

#ifndef HTTP_SERVER_SESSIONMANAGER_HPP
#define HTTP_SERVER_SESSIONMANAGER_HPP

#include <string>
#include <memory>

#include <botan/pk_keys.h>
#include <botan/pubkey.h>
#include <botan/x509_key.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "HandShake.pb.h"

#include "keto/server_session/HttpSession.hpp"

#include "keto/obfuscate/MetaString.hpp"
#include "Constants.hpp"


namespace keto {
namespace server_session {

class HttpSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    friend class HttpRequestManager;
    
    HttpSessionManager();
    HttpSessionManager(const HttpSessionManager& orig) = delete;
    virtual ~HttpSessionManager();

    boost::beast::http::response<boost::beast::http::string_body> processHello(const std::string& hello);
    boost::beast::http::response<boost::beast::http::string_body> processAuthentication(
            boost::beast::http::request<boost::beast::http::string_body>& req,
            const std::string& body);
    
    bool isValid(const std::vector<uint8_t>& sessionHash);
    std::shared_ptr<HttpSession> getSession(const std::vector<uint8_t>& sessionHash);
    
protected:
    
private:
    std::vector<uint8_t> sessionAccount;
    std::map<std::vector<uint8_t>,std::shared_ptr<HttpSession>> clientHashMap;
    std::map<std::vector<uint8_t>,std::shared_ptr<HttpSession>> clientSessionMap;
    
    
    std::shared_ptr<Botan::Public_Key> validateRemoteHash(keto::proto::ClientHello& clientHello);
    boost::beast::http::response<boost::beast::http::string_body> buildResponse(
            const std::string body, int status = 200, const std::string& contentType = std::string(Constants::CONTENT_TYPE::HTML));
    
};


}
}


#endif /* HTTPSESSIONMANAGER_HPP */

