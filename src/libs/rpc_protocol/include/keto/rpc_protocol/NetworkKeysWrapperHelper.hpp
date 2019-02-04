//
// Created by Brett Chaldecott on 2019/01/25.
//

#ifndef KETO_NETWORKKEYSWRAPPERHELPER_HPP
#define KETO_NETWORKKEYSWRAPPERHELPER_HPP

#include <string>
#include <vector>
#include <memory>

#include "KeyStore.pb.h"

#include "keto/crypto/Containers.hpp"

#include "keto/rpc_protocol/NetworkKeyHelper.hpp"


namespace keto {
namespace rpc_protocol {

class NetworkKeysWrapperHelper;
typedef std::shared_ptr<NetworkKeysWrapperHelper> NetworkKeysWrapperHelperPtr;


class NetworkKeysWrapperHelper {
public:
    NetworkKeysWrapperHelper();
    NetworkKeysWrapperHelper(const keto::proto::NetworkKeysWrapper& source);
    NetworkKeysWrapperHelper(const std::string& source);
    NetworkKeysWrapperHelper(const std::vector<uint8_t>& source);
    NetworkKeysWrapperHelper(const NetworkKeysWrapperHelper& orig) = default;
    virtual ~NetworkKeysWrapperHelper();


    NetworkKeysWrapperHelper& setBytes(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> getBytes();

    operator std::vector<uint8_t>();
    NetworkKeysWrapperHelper& operator = (const std::vector<uint8_t>& bytes);

    keto::proto::NetworkKeysWrapper getNetworkKeysWrapper();
    operator keto::proto::NetworkKeysWrapper();
    std::string getNetworkKeysWrapperString();



private:
    keto::proto::NetworkKeysWrapper networkKeysWrapper;
};


}
}


#endif //KETO_NETWORKKEYSWRAPPERHELPER_HPP
