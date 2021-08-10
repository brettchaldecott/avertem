//
// Created by Brett Chaldecott on 2021/07/24.
//

#include "keto/rpc_server/RpcSessionSocket.hpp"
#include "keto/rpc_server/Exception.hpp"
#include "keto/rpc_server/RpcSessionManager.hpp"

#include "keto/common/Log.hpp"
#include "keto/server_common/ServerInfo.hpp"
#include "keto/server_common/Constants.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/rpc_server/Constants.hpp"

#include "keto/rpc_server/RpcServerProtocol.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace sslBeast = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace keto {
namespace rpc_server {

std::string RpcSessionSocket::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSessionSocket::RpcSessionSocket(int sessionId,tcp::socket&& socket, sslBeast::context& ctx) :
    sessionId(sessionId),
    active(true),
    ws_(std::move(socket), ctx) {

    ws_.auto_fragment(false);
}

RpcSessionSocket::~RpcSessionSocket() {

}

void RpcSessionSocket::start(const RpcServerProtocolPtr& rpcServerProtocolPtr) {
    this->rpcServerProtocolPtr = rpcServerProtocolPtr;
    this->run();
}

bool RpcSessionSocket::isActive() {
    return active;
}

void RpcSessionSocket::stop() {
    if (this->rpcServerProtocolPtr->isStarted()) {
        this->rpcServerProtocolPtr->preStop();
        this->rpcServerProtocolPtr->stop();
        this->rpcServerProtocolPtr->join();
    }
}

void RpcSessionSocket::join() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    // wait for this session to end
    while(this->active) {
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_SERVER_QUEUE_DELAY));
    }
}

void RpcSessionSocket::deactivate() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->active = false;
    this->stateCondition.notify_all();
}

void RpcSessionSocket::run() {
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    ws_.next_layer().async_handshake(
            sslBeast::stream_base::server,
            beast::bind_front_handler(
                    &RpcSessionSocket::on_handshake,
                    shared_from_this()));
}

void RpcSessionSocket::on_handshake(boost::system::error_code ec) {
    if(ec) {
        return fail(ec, "handshake");
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(
            websocket::stream_base::timeout::suggested(
                    beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async-ssl");
            }));

    // Accept the websocket handshake
    ws_.async_accept(
            beast::bind_front_handler(
                    &RpcSessionSocket::on_accept,
                    shared_from_this()));
}

void RpcSessionSocket::on_accept(boost::system::error_code ec) {
    if(ec) {
        return fail(ec, "accept");
    }
    this->rpcServerProtocolPtr->start(shared_from_this());
    do_read();
}

void RpcSessionSocket::do_read() {
    // Read a message into our buffer
    ws_.async_read(
            sessionBuffer,
            beast::bind_front_handler(
                    &RpcSessionSocket::on_read,
                    shared_from_this()));
}

void RpcSessionSocket::on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred) {
    keto::server_common::StringVector stringVector;

    boost::ignore_unused(bytes_transferred);

    if (ec) {
        return fail(ec, "read");
    }

    std::stringstream ss;
    ss << boost::beast::make_printable(sessionBuffer.data());
    std::string data = ss.str();
    stringVector = keto::server_common::StringUtils(data).tokenize(" ");

    // Clear the buffer
    sessionBuffer.consume(sessionBuffer.size());
    std::string command = stringVector[0];
    std::string payload;
    if (stringVector.size() == 2) {
        payload = stringVector[1];
    }
    this->rpcServerProtocolPtr->getReceiveQueue()->pushEntry(command, payload);

    // do a read and wait for more messages
    do_read();
}

void RpcSessionSocket::send(const std::string& message) {
    this->outMessage = message;
    net::post(
            ws_.get_executor(),
            beast::bind_front_handler(
                    &RpcSessionSocket::sendMessage,
                    shared_from_this()));
}

void RpcSessionSocket::sendMessage() {
    if (outMessage == keto::server_common::Constants::RPC_COMMANDS::CLOSE) {
        ws_.async_close(websocket::close_code::normal,
                        beast::bind_front_handler(
                                &RpcSessionSocket::on_close,
                                shared_from_this()));
    } else {
        ws_.text(ws_.got_text());
        ws_.async_write(
                boost::asio::buffer(outMessage),
                beast::bind_front_handler(
                        &RpcSessionSocket::on_write,
                        shared_from_this()));
    }
}

void RpcSessionSocket::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    this->outMessage.clear();
    if (ec) {
        return fail(ec, "on_write");
    }
    this->rpcServerProtocolPtr->getSendQueue()->releaseEntry();
}

void RpcSessionSocket::on_close(boost::system::error_code ec) {
    if (this->rpcServerProtocolPtr->isStarted()) {
        this->rpcServerProtocolPtr->stop();
    }
    RpcSessionManager::getInstance()->markAsEndedSession(this->sessionId);
}

void RpcSessionSocket::fail(boost::system::error_code ec, char const* what) {
    KETO_LOG_ERROR << "Failed to process because : " << what << ": " << ec.message();
    deactivate();
    if (this->rpcServerProtocolPtr->isStarted()) {
        this->rpcServerProtocolPtr->abort();
    }
    RpcSessionManager::getInstance()->markAsEndedSession(this->sessionId);
}


}
}
