/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConsensusMessageHelper.hpp
 * Author: ubuntu
 *
 * Created on May 28, 2018, 5:23 PM
 */

#ifndef CONSENSUSMESSAGEHELPER_HPP
#define CONSENSUSMESSAGEHELPER_HPP

#include <vector>
#include <memory>

#include "HandShake.pb.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/crypto/KeyLoader.hpp"
#include "keto/software_consensus/SoftwareMerkelUtils.hpp"

namespace keto {
namespace software_consensus {

class ConsensusMessageHelper;
typedef std::shared_ptr<ConsensusMessageHelper> ConsensusMessageHelperPtr;
    
class ConsensusMessageHelper {
public:
    ConsensusMessageHelper(
            const std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr);
    ConsensusMessageHelper(
            const std::string& consensus);
    ConsensusMessageHelper(const ConsensusMessageHelper& orig) = default;
    virtual ~ConsensusMessageHelper();
    
    ConsensusMessageHelper& setAccountHash(
            const std::vector<uint8_t>& accountHash);
    ConsensusMessageHelper& setAccountHash(const keto::asn1::HashHelper& hashHelper);
    ConsensusMessageHelper& addSystemHash(const keto::asn1::HashHelper& hashHelper);
    ConsensusMessageHelper& generateMerkelRoot();
    ConsensusMessageHelper& sign();
    
    operator keto::proto::ConsensusMessage();
    operator std::string();
    
private:
    std::shared_ptr<keto::crypto::KeyLoader> keyLoaderPtr;
    SoftwareMerkelUtils softwareMerkelUtils;
    keto::proto::ConsensusMessage consensusMessage;
    
    
};


}
}


#endif /* CONSENSUSMESSAGEHELPER_HPP */

