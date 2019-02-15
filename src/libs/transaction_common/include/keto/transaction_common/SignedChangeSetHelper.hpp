//
// Created by Brett Chaldecott on 2019/02/13.
//

#ifndef KETO_SIGNEDCHANGESETHELPER_HPP
#define KETO_SIGNEDCHANGESETHELPER_HPP

#include <string>
#include <memory>

#include "ChangeSet.h"
#include "SignedChangeSet.h"

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/ChangeSetHelper.hpp"

namespace keto {
namespace transaction_common {

class SignedChangeSetHelper;
typedef std::shared_ptr<SignedChangeSetHelper> SignedChangeSetHelperPtr;

class SignedChangeSetHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    SignedChangeSetHelper(SignedChangeSet_t* signedChangedSet);
    SignedChangeSetHelper(const SignedChangeSetHelper& signedChangedSetHelper) = delete;
    virtual ~SignedChangeSetHelper();

    operator SignedChangeSet_t*();
    operator SignedChangeSet_t&();

    keto::asn1::ChangeSetHelperPtr getChangeSetHelper();

    keto::asn1::HashHelper getHash();

private:
    SignedChangeSet_t* signedChangedSet;
};


}
}



#endif //KETO_SIGNEDCHANGESETHELPER_HPP
