//
// Created by Brett Chaldecott on 2021/07/19.
//

#include "include/keto/rpc_server/RpcListener.hpp"

#include "keto/rpc_server/Exception.hpp"
#include "keto/rpc_server/RpcSessionManager.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

std::string RpcListener::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcListener::RpcListener(
        std::shared_ptr<boost::asio::io_context> ioc,
        std::shared_ptr<sslBeast::context> ctx,
        tcp::endpoint endpoint) :
        active(true)
        , ioc_(ioc)
        , ctx_(ctx)
        , acceptor_(net::make_strand(*ioc)) {
    boost::system::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec)
    {
        fail(ec, "open");
        BOOST_THROW_EXCEPTION(ListenerFailedToStart());
    }

    // this solves the shut down problem
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true),ec);
    if(ec)
    {
        fail(ec, "set_option");
        BOOST_THROW_EXCEPTION(ListenerFailedToStart());
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec)
    {
        fail(ec, "bind");
        BOOST_THROW_EXCEPTION(ListenerFailedToStart());
    }

    // Start listening for connections
    acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
    if(ec)
    {
        fail(ec, "listen");
        BOOST_THROW_EXCEPTION(ListenerFailedToStart());
    }
}

RpcListener::~RpcListener() {

}

void RpcListener::start() {
    this->run();
}

void RpcListener::stop() {
    // close acceptor
    acceptor_.close();
}

void RpcListener::run() {
    if(! acceptor_.is_open()) {
        KETO_LOG_ERROR << "Failed to start the acceptor";
        return;
    }
    do_accept();
}

void RpcListener::do_accept() {
    // The new connection gets its own strand
    acceptor_.async_accept(
            net::make_strand(*ioc_),
            beast::bind_front_handler(
                    &RpcListener::on_accept,
                    shared_from_this()));
}

void RpcListener::on_accept(boost::system::error_code ec, tcp::socket socket) {
    if(ec) {
        return fail(ec, "accept");
    } else {
        RpcSessionManager::getInstance()->addSession(std::move(socket), *ctx_);
    }
    // Accept another connection
    do_accept();
}

void RpcListener::fail(boost::system::error_code ec, char const* what) {
    KETO_LOG_ERROR << "Failed to accept because : " << what << ": " << ec.message();
}

}
}
