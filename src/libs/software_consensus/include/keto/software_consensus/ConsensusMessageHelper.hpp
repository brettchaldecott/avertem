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
#include "keto/software_consensus/SoftwareConsensusHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ConsensusMessageHelper;
typedef std::shared_ptr<ConsensusMessageHelper> ConsensusMessageHelperPtr;
    
class ConsensusMessageHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    ConsensusMessageHelper();
    ConsensusMessageHelper(
            const std::string& consensus);
    ConsensusMessageHelper(
            const keto::proto::ConsensusMessage& consensusMessage);
    ConsensusMessageHelper(const ConsensusMessageHelper& orig) = default;
    virtual ~ConsensusMessageHelper();
    
    ConsensusMessageHelper& setAccountHash(
            const std::vector<uint8_t>& accountHash);
    ConsensusMessageHelper& setAccountHash(const keto::asn1::HashHelper& hashHelper);
    
    ConsensusMessageHelper& setMsg(keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper);
    keto::software_consensus::SoftwareConsensusHelper getMsg();
    
    operator keto::proto::ConsensusMessage();
    operator std::string();
    
private:
    keto::proto::ConsensusMessage consensusMessage;
    
    
};


}
}


#endif /* CONSENSUSMESSAGEHELPER_HPP */

