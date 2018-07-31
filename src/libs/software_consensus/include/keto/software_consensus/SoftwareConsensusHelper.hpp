/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SoftwareConsensusHelper.hpp
 * Author: ubuntu
 *
 * Created on June 14, 2018, 10:50 AM
 */

#ifndef SOFTWARECONSENSUSHELPER_HPP
#define SOFTWARECONSENSUSHELPER_HPP

#include <string>
#include <memory>

#include "SoftwareConsensus.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"

#include "keto/crypto/KeyLoader.hpp"


namespace keto {
namespace software_consensus {


class SoftwareConsensusHelper {
public:
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id: cd6f953fdc6d6011f27667fc3267cb9f0e6fa962 $");
    };
    
    static std::string getSourceVersion();

    
    SoftwareConsensusHelper();
    SoftwareConsensusHelper(const SoftwareConsensus_t& orig);
    SoftwareConsensusHelper(const std::string& orig);
    SoftwareConsensusHelper(const SoftwareConsensusHelper& orig);
    virtual ~SoftwareConsensusHelper();
    
    SoftwareConsensusHelper& setDate(const keto::asn1::TimeHelper& date);
    SoftwareConsensusHelper& setPreviousHash(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getAccount() const;
    SoftwareConsensusHelper& setAccount(const keto::asn1::HashHelper& hashHelper);
    SoftwareConsensusHelper& setSeed(const keto::asn1::HashHelper& hashHelper);
    SoftwareConsensusHelper& addSystemHash(const keto::asn1::HashHelper& hashHelper);
    
    SoftwareConsensusHelper& generateMerkelRoot();
    SoftwareConsensusHelper& sign(
            const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr);
    
    operator SoftwareConsensus_t*();
    
    
private:
    SoftwareConsensus_t* softwareConsensus;
    
};


}
}
#endif /* SOFTWARECONSENSUSHELPER_HPP */

