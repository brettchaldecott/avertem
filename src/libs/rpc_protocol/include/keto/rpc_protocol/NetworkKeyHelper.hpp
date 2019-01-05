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


    keto::crypto::SecureVector getAccountHash();
    NetworkKeyHelper& setAccountHash(const keto::crypto::SecureVector& accountHash);


    keto::crypto::SecureVector getKeyBytes();
    NetworkKeyHelper& setKeyBytes(const keto::crypto::SecureVector& bytes);


    operator keto::proto::NetworkKey();

private:
    keto::proto::NetworkKey networkKey;
};


}
}


#endif //KETO_NETWORKKEYHELPER_HPP
