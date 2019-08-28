//
// Created by Brett Chaldecott on 2019-08-21.
//

#ifndef KETO_ELECTIONHELPER_HPP
#define KETO_ELECTIONHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.h"

#include "keto/asn1/NumberHelper.hpp"
#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/HashHelper.hpp"

#include "keto/crypto/Containers.hpp"

#include "keto/software_consensus/SoftwareConsensusHelper.hpp"


namespace keto {
namespace election_common {

class ElectionHelper;
typedef std::shared_ptr<ElectionHelper> ElectionHelperPtr;

class ElectionHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionHelper();
    ElectionHelper(const Election_t* election);
    ElectionHelper(const Election_t& election);
    ElectionHelper(const ElectionHelper& electionHelper);
    virtual ~ElectionHelper();

    long getVersion();
    keto::asn1::TimeHelper getDate();
    ElectionHelper& setDate(const keto::asn1::TimeHelper& timeHelper);

    keto::asn1::HashHelper getAccountHash();
    ElectionHelper& setAccountHash(const keto::asn1::HashHelper& accountHash);

    keto::software_consensus::SoftwareConsensusHelper getAcceptedCheck();
    ElectionHelper& setAcceptedCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper);

    keto::software_consensus::SoftwareConsensusHelper getValidateCheck();
    ElectionHelper& setValidateCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper);

    operator Election_t*();
    operator Election_t() const;
    Election_t* getElection();

    operator std::vector<uint8_t>();
    operator keto::crypto::SecureVector();

private:
    Election_t* election;

};

}
}


#endif //KETO_ELECTIONHELPER_HPP
