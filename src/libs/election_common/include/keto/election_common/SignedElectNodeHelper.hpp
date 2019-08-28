//
// Created by Brett Chaldecott on 2019-08-27.
//

#ifndef KETO_SIGNEDELECTNODEHELPER_HPP
#define KETO_SIGNEDELECTNODEHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "SignedElectNode.h"

#include "keto/asn1/NumberHelper.hpp"
#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/HashHelper.hpp"

#include "keto/crypto/Containers.hpp"

#include "keto/software_consensus/SoftwareConsensusHelper.hpp"

#include "keto/election_common/ElectNodeHelper.hpp"

namespace keto {
namespace election_common {

class SignedElectNodeHelper;
typedef std::shared_ptr<SignedElectNodeHelper> SignedElectNodeHelperPtr;

class SignedElectNodeHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    SignedElectNodeHelper();
    SignedElectNodeHelper(const SignedElectNode_t* signedElectNode);
    SignedElectNodeHelper(const SignedElectNode_t& signedElectNode);
    SignedElectNodeHelper(const SignedElectNodeHelper& signedElectNodeHelper);
    virtual ~SignedElectNodeHelper();

    long getVersion();

    ElectNodeHelperPtr getElectedNode();
    SignedElectNodeHelper& setElectedNode(const ElectNodeHelper& electNodeHelper);

    keto::asn1::HashHelper getElectedHash();

    keto::asn1::SignatureHelper getSignature();
    void sign(const keto::asn1::PrivateKeyHelper& privateKeyHelper);
    void sign(const keto::crypto::SecureVector& key);
    void sign(const keto::crypto::KeyLoaderPtr privateKey);

    operator SignedElectNode_t*() const;
    operator SignedElectNode_t() const;
    SignedElectNode_t* getElection();

    operator std::vector<uint8_t>();
    operator keto::crypto::SecureVector();

private:
    SignedElectNode_t* signedElectNode;
};


}
}


#endif //KETO_SIGNEDELECTNODEHELPER_HPP
