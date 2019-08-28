//
// Created by Brett Chaldecott on 2019-08-22.
//

#ifndef KETO_SIGNEDELECTION_HPP
#define KETO_SIGNEDELECTION_HPP


#include <string>
#include <map>
#include <memory>
#include <vector>

#include "SignedElection.h"

#include "keto/asn1/NumberHelper.hpp"
#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/PrivateKeyHelper.hpp"

#include "keto/software_consensus/SoftwareConsensusHelper.hpp"

#include "keto/election_common/ElectionHelper.hpp"

namespace keto {
namespace election_common {

class SignedElectionHelper;
typedef std::shared_ptr<SignedElectionHelper> SignedElectionHelperPtr;


class SignedElectionHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    SignedElectionHelper();
    SignedElectionHelper(const SignedElection_t* signedElection);
    SignedElectionHelper(const SignedElection_t& signedElection);
    SignedElectionHelper(const std::string& signedElection);
    SignedElectionHelper(const SignedElectionHelper& signedElectionHelper);
    virtual ~SignedElectionHelper();

    long getVersion();

    SignedElectionHelper& setElectionHelper(const ElectionHelper& electionHelper);
    ElectionHelperPtr getElectionHelper();

    keto::asn1::HashHelper getElectionHash();

    keto::asn1::SignatureHelper getSignature();
    void sign(const keto::asn1::PrivateKeyHelper& privateKeyHelper);
    void sign(const keto::crypto::SecureVector& key);
    void sign(const keto::crypto::KeyLoaderPtr privateKey);

    operator std::vector<uint8_t>() const;
    operator keto::crypto::SecureVector() const;
    operator std::string() const;

    operator SignedElection_t*() const;
    operator SignedElection_t() const;
private:
    SignedElection_t* signedElection;

};


}
}



#endif //KETO_SIGNEDELECTION_HPP
