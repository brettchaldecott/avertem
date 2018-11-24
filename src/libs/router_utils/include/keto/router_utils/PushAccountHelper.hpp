/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PushAccountHelper.hpp
 * Author: ubuntu
 *
 * Created on August 22, 2018, 6:51 PM
 */

#ifndef PUSHACCOUNTHELPER_HPP
#define PUSHACCOUNTHELPER_HPP

#include <string>
#include <memory>

#include "Route.pb.h"

#include "keto/asn1/HashHelper.hpp"

#include "keto/common/MetaInfo.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace router_utils {

class PushAccountHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    PushAccountHelper();
    PushAccountHelper(const keto::proto::PushAccount& orig);
    PushAccountHelper(const std::string& value);
    PushAccountHelper(const PushAccountHelper& orig) = default;
    virtual ~PushAccountHelper();
    
    
    PushAccountHelper& setAccountHash(
            const keto::asn1::HashHelper& accountHash);
    keto::asn1::HashHelper getAccountHash();
    PushAccountHelper& setAccountHash(
            const std::vector<uint8_t>& accountHash);
    std::vector<uint8_t> getAccountHashBytes();
    PushAccountHelper& setAccountHash(
            const std::string& accountHash);
    std::string getAccountHashString();
    
    PushAccountHelper& setManagementAccountHash(
            const keto::asn1::HashHelper& accountHash);
    keto::asn1::HashHelper getManagementAccountHash();
    PushAccountHelper& setManagementAccountHash(
            const std::vector<uint8_t>& accountHash);
    std::vector<uint8_t> getManagementAccountHashBytes();
    PushAccountHelper& setManagementAccountHash(
        const std::string& accountHash);
    std::string getManagementAccountHashString();
    
    PushAccountHelper& addChildAccount(
            const keto::proto::PushAccount& child);
    PushAccountHelper& addChildAccount(
            PushAccountHelper& child);
    std::vector<PushAccountHelper> getChildren();
    
    
    operator keto::proto::PushAccount();
    std::string toString();
    
private:
    keto::proto::PushAccount pushAccount;
};

}
}


#endif /* PUSHACCOUNTHELPER_HPP */

