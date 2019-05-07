//
// Created by Brett Chaldecott on 2019-05-06.
//

#include <boost/algorithm/string.hpp>

#include "keto/rpc_client/RpcPeer.hpp"
#include "keto/rpc_client/Constants.hpp"


namespace keto {
namespace rpc_client {

bool operator < (const RpcPeer& lhs, const RpcPeer& rhs) {
    return lhs.getPeer() < rhs.getPeer();
}

bool operator == (const RpcPeer& lhs, const RpcPeer& rhs) {
    return lhs.getPeer() == rhs.getPeer();
}

std::string RpcPeer::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RpcPeer::RpcPeer(const std::string& peer, bool peered) : peer(peer), peered(peered), reconnectCount(0) {
    this->host = peer;
    if (host.find(":") != std::string::npos) {
        std::vector<std::string> results;
        boost::split(results, host, [](char c){return c == ':';});
        this->host = results[0];
        this->port = results[1];
    } else {
        this->port = Constants::DEFAULT_PORT_NUMBER;
    }
}

RpcPeer::~RpcPeer() {

}

std::string RpcPeer::getPeer() const {
    return this->peer;
}

std::string RpcPeer::getHost() const {
    return this->host;
}

std::string RpcPeer::getPort() const {
    return this->port;
}

bool RpcPeer::getPeered() const {
    return this->peered;
}

void RpcPeer::setPeered(bool peered) {
    this->peered = peered;
}


int RpcPeer::getReconnectCount() const {
    return this->reconnectCount;
}

int RpcPeer::incrementReconnectCount() {
    return this->reconnectCount++;
}


RpcPeer::operator std::string () const {
    return this->peer;
}

}
}
