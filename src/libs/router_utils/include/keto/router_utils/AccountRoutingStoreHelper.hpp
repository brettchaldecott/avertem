/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountRoutingStoreHelper.hpp
 * Author: ubuntu
 *
 * Created on August 22, 2018, 10:14 AM
 */

#ifndef ACCOUNTROUTINGSTOREHELPER_HPP
#define ACCOUNTROUTINGSTOREHELPER_HPP

#include <string>
#include <memory>

#include "Route.pb.h"

#include "keto/asn1/HashHelper.hpp"

#include "keto/common/MetaInfo.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace router_utils {


class AccountRoutingStoreHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    
    AccountRoutingStoreHelper();
    AccountRoutingStoreHelper(const keto::proto::AccountRoutingStore& orig);
    AccountRoutingStoreHelper(const std::string& value);
    AccountRoutingStoreHelper(const AccountRoutingStoreHelper& orig) = default;
    virtual ~AccountRoutingStoreHelper();
    
    AccountRoutingStoreHelper& setManagementAccountHash(
            const keto::asn1::HashHelper& accountHash);
    keto::asn1::HashHelper getManagementAccountHash();
    AccountRoutingStoreHelper& setManagementAccountHash(
            const std::vector<uint8_t>& accountHash);
    std::vector<uint8_t> getManagementAccountHashBytes();
    
    std::string toString();
    operator keto::proto::AccountRoutingStore();
    
private:
    keto::proto::AccountRoutingStore accountRoutingStore;
    
};


}
}

#endif /* ACCOUNTROUTINGSTOREHELPER_HPP */

