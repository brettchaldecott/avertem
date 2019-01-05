//
// Created by Brett Chaldecott on 2018/12/21.
//

#ifndef KETO_NETWORKKEYSHELPER_HPP
#define KETO_NETWORKKEYSHELPER_HPP

#include "KeyStore.pb.h"

#include "keto/crypto/Containers.hpp"

#include "keto/rpc_protocol/NetworkKeyHelper.hpp"

namespace keto {
namespace rpc_protocol {

class NetworkKeysHelper {
public:
    NetworkKeysHelper();
    NetworkKeysHelper(const keto::proto::NetworkKeys& networkKeys);
    NetworkKeysHelper(const NetworkKeysHelper& orig) = default;
    virtual ~NetworkKeysHelper();

    NetworkKeysHelper& addNetworkKey(const keto::proto::NetworkKey& networkKey);
    NetworkKeysHelper& addNetworkKey(const NetworkKeyHelper& networkKeyHelper);
    std::vector<NetworkKeyHelper> getNetworkKeys();

    operator keto::proto::NetworkKeys();

private:
    keto::proto::NetworkKeys networkKeys;
};


}
}



#endif //KETO_NETWORKKEYSHELPER_HPP
