//
// Created by Brett Chaldecott on 2020/07/22.
//

#ifndef KETO_NETWORKSTATUSHELPER_HPP
#define KETO_NETWORKSTATUSHELPER_HPP

#include <string>
#include <memory>

#include "Route.pb.h"

#include "keto/crypto/Containers.hpp"
#include "keto/asn1/HashHelper.hpp"

namespace keto {
namespace rpc_protocol {

class NetworkStatusHelper {
public:
    NetworkStatusHelper();
    NetworkStatusHelper(const keto::proto::NetworkStatus& networkStatus);
    NetworkStatusHelper(const std::string& networkStatus);
    NetworkStatusHelper(const NetworkStatusHelper& networkStatusHelper) = default;
    virtual ~NetworkStatusHelper();

    NetworkStatusHelper& addAccount(const keto::asn1::HashHelper& account);
    std::vector<keto::asn1::HashHelper> getAccounts();

    NetworkStatusHelper& setSlot(int slot);
    int getSlot();

    operator keto::proto::NetworkStatus() const;
    operator std::string() const;
    operator keto::crypto::SecureVector() const;

private:
    keto::proto::NetworkStatus networkStatus;
};

}
}


#endif //KETO_NETWORKSTATUSHELPER_HPP
