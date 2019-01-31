//
// Created by Brett Chaldecott on 2018/12/21.
//

#ifndef KETO_NETWORKKEYHELPER_HPP
#define KETO_NETWORKKEYHELPER_HPP

#include "KeyStore.pb.h"

#include "keto/crypto/Containers.hpp"

namespace keto {
namespace rpc_protocol {

class NetworkKeyHelper {
public:
    NetworkKeyHelper();
    NetworkKeyHelper(const keto::proto::NetworkKey& networkKey);
    NetworkKeyHelper(const NetworkKeyHelper& orig) = default;
    virtual ~NetworkKeyHelper();

    NetworkKeyHelper& setHash(const std::vector<uint8_t>& hash);
    std::vector<uint8_t> getHash();
    NetworkKeyHelper& setHash(const keto::crypto::SecureVector& hash);
    keto::crypto::SecureVector getHash_locked();


    std::vector<uint8_t> getKeyBytes();
    NetworkKeyHelper& setKeyBytes(const std::vector<uint8_t>& bytes);
    keto::crypto::SecureVector getKeyBytes_locked();
    NetworkKeyHelper& setKeyBytes(const keto::crypto::SecureVector& bytes);

    bool getActive();
    NetworkKeyHelper& setActive(bool active);

    keto::proto::NetworkKey getNetworkKey() const;
    operator keto::proto::NetworkKey() const;

private:
    keto::proto::NetworkKey networkKey;
};


}
}


#endif //KETO_NETWORKKEYHELPER_HPP
