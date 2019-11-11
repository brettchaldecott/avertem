//
// Created by Brett Chaldecott on 2019-08-26.
//

#ifndef KETO_ELECTNODEHELPER_HPP
#define KETO_ELECTNODEHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "ElectNode.h"

#include "keto/asn1/NumberHelper.hpp"
#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/HashHelper.hpp"

#include "keto/crypto/Containers.hpp"

#include "keto/software_consensus/SoftwareConsensusHelper.hpp"

#include "keto/election_common/SignedElectionHelper.hpp"


namespace keto {
namespace election_common {

class ElectNodeHelper;
typedef std::shared_ptr<ElectNodeHelper> ElectNodeHelperPtr;

class ElectNodeHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectNodeHelper();
    ElectNodeHelper(const ElectNode_t* electNode);
    ElectNodeHelper(const ElectNode_t& electNode);
    ElectNodeHelper(const ElectNodeHelper& electNodeHelper);
    virtual ~ElectNodeHelper();

    long getVersion();
    keto::asn1::TimeHelper getDate();
    ElectNodeHelper& setDate(const keto::asn1::TimeHelper& timeHelper);

    keto::asn1::HashHelper getAccountHash();
    ElectNodeHelper& setAccountHash(const keto::asn1::HashHelper& hashHelper);

    SignedElectionHelperPtr getElectedNode();
    ElectNodeHelper& setElectedNode(const SignedElectionHelper& signedElectionHelper);

    std::vector<SignedElectionHelperPtr> getAlternatives();
    ElectNodeHelper& addAlternative(const SignedElectionHelper& timeHelper);

    keto::software_consensus::SoftwareConsensusHelper getAcceptedCheck();
    ElectNodeHelper& setAcceptedCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper);

    keto::software_consensus::SoftwareConsensusHelper getValidateCheck();
    ElectNodeHelper& setValidateCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper);


    operator ElectNode_t*();
    operator ElectNode_t() const;
    ElectNode_t* getElection();

    operator std::vector<uint8_t>();
    operator keto::crypto::SecureVector();



private:
    ElectNode_t* electNode;


};


}
}



#endif //KETO_ELECTNODEHELPER_HPP
