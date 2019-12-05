//
// Created by Brett Chaldecott on 2018/12/21.
//

#ifndef KETO_NETWORKKEYSHELPER_HPP
#define KETO_NETWORKKEYSHELPER_HPP

#include <string>

#include "KeyStore.pb.h"

#include "keto/crypto/Containers.hpp"

#include "keto/rpc_protocol/NetworkKeyHelper.hpp"

namespace keto {
namespace rpc_protocol {

class NetworkKeysHelper {
public:
    NetworkKeysHelper();
    NetworkKeysHelper(const keto::proto::NetworkKeys& networkKeys);
    NetworkKeysHelper(const std::string& networkKeys);
    NetworkKeysHelper(const NetworkKeysHelper& orig) = default;
    virtual ~NetworkKeysHelper();

    NetworkKeysHelper& addNetworkKey(const keto::proto::NetworkKey& networkKey);
    NetworkKeysHelper& addNetworkKey(const NetworkKeyHelper& networkKeyHelper);
    std::vector<NetworkKeyHelper> getNetworkKeys() const;

    NetworkKeysHelper& setSlot(int slot);
    int getSlot();

    operator keto::proto::NetworkKeys();
    operator std::string();
    operator keto::crypto::SecureVector();

private:
    keto::proto::NetworkKeys networkKeys;
};


}
}



#endif //KETO_NETWORKKEYSHELPER_HPP
