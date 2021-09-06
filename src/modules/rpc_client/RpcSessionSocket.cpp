//
// Created by Brett Chaldecott on 2021/08/11.
//

#include "keto/rpc_client/RpcSessionSocket.hpp"

#include "keto/server_common/Constants.hpp"
#include "keto/rpc_client/Constants.hpp"
#include "keto/rpc_client/Exception.hpp"
#include "keto/rpc_client/RpcClientProtocol.hpp"
#include "keto/rpc_client/RpcReceiveQueue.hpp"
#include "keto/rpc_client/RpcSendQueue.hpp"
#include "keto/rpc_client/RpcSessionManager.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"
#include "keto/common/Log.hpp"


namespace keto {
namespace rpc_client {

std::string RpcSessionSocket::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcSessionSocket::RpcSessionSocket(int sessionId,
             std::shared_ptr<net::io_context>& ioc,
             std::shared_ptr<sslBeast::context>& ctx,
             const RpcPeer& rpcPeer) : sessionId(sessionId), active(true), reconnect(true),
             resolver(net::make_strand(*ioc)),
             ws_(net::make_strand(*ioc), *ctx),
             rpcPeer(rpcPeer) {

    KETO_LOG_INFO << "[RpcSession::RpcSession] Start a new session for : " << rpcPeer.getHost();
    //this->sessionNumber = sessionIndex++;
    ws_.auto_fragment(false);
    // setup the key loader
    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::PRIVATE_KEY)) {
        BOOST_THROW_EXCEPTION(keto::rpc_client::PrivateKeyNotConfiguredException());
    }
    std::string privateKeyPath =
            config->getVariablesMap()[Constants::PRIVATE_KEY].as<std::string>();
    if (!config->getVariablesMap().count(Constants::PUBLIC_KEY)) {
        BOOST_THROW_EXCEPTION(keto::rpc_client::PrivateKeyNotConfiguredException());
    }
    std::string publicKeyPath =
            config->getVariablesMap()[Constants::PUBLIC_KEY].as<std::string>();
    keyLoaderPtr = std::make_shared<keto::crypto::KeyLoader>(privateKeyPath,
                                                             publicKeyPath);

    KETO_LOG_INFO << "[RpcSession::RpcSession] New session created for host : " << rpcPeer.getHost();
}

RpcSessionSocket::~RpcSessionSocket() {

}

void RpcSessionSocket::start(const RpcClientProtocolPtr& rpcClientProtocolPtr) {
    this->rpcClientProtocolPtr = rpcClientProtocolPtr;
    this->run();
}

bool RpcSessionSocket::isActive() {
    return active;
}

void RpcSessionSocket::stop() {
    if (this->rpcClientProtocolPtr->isStarted()) {
        this->rpcClientProtocolPtr->preStop();
        this->rpcClientProtocolPtr->stop();
        this->rpcClientProtocolPtr->join();
    }
}

void RpcSessionSocket::join() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    // wait for this session to end
    while(this->active) {
        this->stateCondition.wait_for(uniqueLock, std::chrono::seconds(
                Constants::DEFAULT_RPC_CLIENT_QUEUE_DELAY));
    }
}

void RpcSessionSocket::send(const std::string& message) {
    this->outMessage = message;
    net::post(
            ws_.get_executor(),
            beast::bind_front_handler(
                    &RpcSessionSocket::sendMessage,
                    shared_from_this()));
}

bool RpcSessionSocket::isReconnect() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    return this->reconnect;
}

void RpcSessionSocket::setReconnect(bool reconnect) {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->reconnect = reconnect;
}

void RpcSessionSocket::deactivate() {
    std::unique_lock<std::mutex> uniqueLock(classMutex);
    this->active = false;
    this->stateCondition.notify_all();
}

void RpcSessionSocket::run() {
    this->resolver.async_resolve(
            this->rpcPeer.getHost().c_str(),
            this->rpcPeer.getPort().c_str(),
            beast::bind_front_handler(
                    &RpcSessionSocket::on_resolve,
                    shared_from_this()));
}

void RpcSessionSocket::on_resolve(boost::system::error_code ec, tcp::resolver::results_type results) {
    if(ec) {
        return fail(ec, Constants::SESSION::RESOLVE);
    }
    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                    &RpcSessionSocket::on_connect,
                    shared_from_this()));
}

void RpcSessionSocket::on_connect(boost::system::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if(ec) {
        return fail(ec, Constants::SESSION::CONNECT);
    }
    KETO_LOG_INFO << "[RpcSession::on_connect] connecting to host : " << rpcPeer.getHost();
    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(60));

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
            sslBeast::stream_base::client,
            beast::bind_front_handler(
                    &RpcSessionSocket::on_ssl_handshake,
                    shared_from_this()));
}

void RpcSessionSocket::on_ssl_handshake(boost::system::error_code ec) {
    if(ec) {
        return fail(ec, Constants::SESSION::SSL_HANDSHAKE);
    }
    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(
            websocket::stream_base::timeout::suggested(
                    beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

    // Perform the websocket handshake
    ws_.async_handshake(rpcPeer.getHost(), "/",
                        beast::bind_front_handler(
                                &RpcSessionSocket::on_handshake,
                                shared_from_this()));
}

void RpcSessionSocket::on_handshake(boost::system::error_code ec) {
    if(ec) {
        return fail(ec, Constants::SESSION::HANDSHAKE);
    }
    KETO_LOG_INFO << "[RpcSession::on_handshake] connection complete start protocol : " << rpcPeer.getHost();

    // trigger the hello processing
    this->rpcClientProtocolPtr->start(shared_from_this());
    this->rpcClientProtocolPtr->getReceiveQueue()->pushEntry(keto::server_common::Constants::RPC_COMMANDS::HELLO,keto::server_common::Constants::RPC_COMMANDS::HELLO, "");

    // do the read
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

void RpcSessionSocket::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        return fail(ec, "read");
    }

    // parse the input
    std::stringstream ss;
    ss << boost::beast::make_printable(sessionBuffer.data());
    std::string data = ss.str();
    keto::server_common::StringVector stringVector = keto::server_common::StringUtils(data).tokenize(" ");
    sessionBuffer.consume(sessionBuffer.size());
    std::string command = stringVector[0];
    std::string payload;
    std::string misc;
    if (stringVector.size() >= 2) {
        payload = stringVector[1];
    }
    if (stringVector.size() >= 3) {
        misc = stringVector[2];
    }
    // Clear the buffer
    sessionBuffer.consume(sessionBuffer.size());

    this->rpcClientProtocolPtr->getReceiveQueue()->pushEntry(command, payload, misc);

    // do a read and wait for more messages
    do_read();
}

void RpcSessionSocket::sendMessage() {
    if (outMessage.find(keto::server_common::Constants::RPC_COMMANDS::CLOSE,0) == 0 ||
    outMessage.find(keto::server_common::Constants::RPC_COMMANDS::CLOSE_EXIT,0) == 0) {
        // check if the reconnect needs to be set as this is required on close exit
        if (outMessage.find(keto::server_common::Constants::RPC_COMMANDS::CLOSE_EXIT,0) == 0) {
            setReconnect(false);
        }
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

void RpcSessionSocket::on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    this->outMessage.clear();
    if (ec) {
        return fail(ec, "on_write");
    }
    this->rpcClientProtocolPtr->getSendQueue()->releaseEntry();
}

void RpcSessionSocket::on_close(boost::system::error_code ec) {
    if (ec) {
        return fail(ec, "on_write");
    }
    deactivate();
    KETO_LOG_INFO << "[RpcSessionSocket::on_close][" << this->sessionId << "][" << this->rpcPeer.getHost() << "] Connection has been closed";
    if (this->rpcClientProtocolPtr->isStarted()) {
        this->rpcClientProtocolPtr->stop();
    }
    KETO_LOG_INFO << "[RpcSessionSocket::on_close][" << this->sessionId << "][" << this->rpcPeer.getHost() << "] Mark close session";
    RpcSessionManager::getInstance()->markAsEndedSession(this->sessionId);
    if (isReconnect()) {
        setReconnect(false);
        this->rpcPeer.incrementReconnectCount();
        std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));
        KETO_LOG_INFO << "[RpcSessionSocket::on_close][" << this->sessionId << "][" << this->rpcPeer.getHost() << "] Add session to the peer";
        RpcSessionManager::getInstance()->addSession(this->rpcPeer);
    }
}

void RpcSessionSocket::fail(boost::system::error_code ec, const std::string& what) {
    KETO_LOG_ERROR << "Failed to process because : " << what << ": " << ec.message();
    deactivate();
    if (this->rpcClientProtocolPtr->isStarted()) {
        this->rpcClientProtocolPtr->abort();
    }
    KETO_LOG_INFO << "[RpcSessionSocket::fail][" << this->sessionId << "][" << this->rpcPeer.getHost() << "] Mark session as ended";
    RpcSessionManager::getInstance()->markAsEndedSession(this->sessionId);
    if (isReconnect()) {
        setReconnect(false);
        this->rpcPeer.incrementReconnectCount();
        std::this_thread::sleep_for(std::chrono::milliseconds(Constants::SESSION::RETRY_COUNT_DELAY));
        KETO_LOG_INFO << "[RpcSessionSocket::fail][" << this->sessionId << "][" << this->rpcPeer.getHost() << "] add session because of fail";
        RpcSessionManager::getInstance()->addSession(this->rpcPeer);
    }
}

}
}
