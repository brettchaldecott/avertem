//
// Created by Brett Chaldecott on 2019-05-06.
//

#ifndef KETO_RPCPEER_HPP
#define KETO_RPCPEER_HPP

#include <string>
#include <map>
#include <memory>

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace rpc_client {

class RpcPeer;
bool operator < (const RpcPeer& lhs, const RpcPeer& rhs);
bool operator == (const RpcPeer& lhs, const RpcPeer& rhs);

class RpcPeer {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    RpcPeer(const std::string& peer, bool peered = false);
    RpcPeer(const RpcPeer& orig) = default;
    virtual ~RpcPeer();

    std::string getPeer() const;
    std::string getHost() const;
    std::string getPort() const;
    bool getPeered() const;
    void setPeered(bool peered);

    int getReconnectCount() const;
    int incrementReconnectCount();

    operator std::string () const;

private:
    std::string peer;
    std::string host;
    std::string port;
    bool peered;
    int reconnectCount;
};


}
}


#endif //KETO_RPCPEER_HPP
