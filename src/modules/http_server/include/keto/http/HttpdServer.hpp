/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HttpdServer.hpp
 * Author: ubuntu
 *
 * Created on January 21, 2018, 5:10 AM
 */

#ifndef KETO_HTTPDSERVER_HPP
#define KETO_HTTPDSERVER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <boost/filesystem/path.hpp>

#include "keto/common/MetaInfo.hpp"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace httpBeast = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace http {

class HttpdServer {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    HttpdServer();
    HttpdServer(const HttpdServer& orig) = delete;
    virtual ~HttpdServer();
    
    void start();
    void preStop();
    void stop();
    
private:
    
    boost::filesystem::path documentRoot;
    boost::asio::ip::address serverIp;
    unsigned short serverPort;
    int threads;
    std::string certPath;
    std::string keyPath;
    std::shared_ptr<sslBeast::context> contextPtr;
    std::shared_ptr<net::io_context> ioc;
    std::vector<std::thread> threadsVector;
};


}
}

#endif /* HTTPDSERVER_HPP */

