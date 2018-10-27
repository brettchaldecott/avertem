/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SoftwareMerkelUtils.hpp
 * Author: ubuntu
 *
 * Created on May 29, 2018, 2:56 AM
 */

#ifndef SOFTWAREMERKELUTILS_HPP
#define SOFTWAREMERKELUTILS_HPP

#include <vector>

#include "keto/asn1/HashHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {


class SoftwareMerkelUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();


    SoftwareMerkelUtils();
    SoftwareMerkelUtils(const SoftwareMerkelUtils& orig) = default;
    virtual ~SoftwareMerkelUtils();
    
    void addHash(const keto::asn1::HashHelper& hash);
    keto::asn1::HashHelper computation();
    
    
private:
    std::vector<keto::asn1::HashHelper> hashs;
    
    std::vector<keto::asn1::HashHelper> sort(std::vector<keto::asn1::HashHelper> hashs);
    keto::asn1::HashHelper compute(std::vector<keto::asn1::HashHelper> hashs);
    keto::asn1::HashHelper compute(keto::asn1::HashHelper lhs, keto::asn1::HashHelper rhs);
};


}
}

#endif /* SOFTWAREMERKELUTILS_HPP */

