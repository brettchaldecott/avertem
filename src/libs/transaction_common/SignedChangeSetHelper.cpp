//
// Created by Brett Chaldecott on 2019/02/13.
//

#include "keto/transaction_common/Exception.hpp"
#include "keto/transaction_common/SignedChangeSetHelper.hpp"
#include "SignedChangeSet.h"


namespace keto {
namespace transaction_common {

std::string SignedChangeSetHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SignedChangeSetHelper::SignedChangeSetHelper(SignedChangeSet_t* signedChangedSet) : signedChangedSet(signedChangedSet) {

}

SignedChangeSetHelper::~SignedChangeSetHelper() {

}

SignedChangeSetHelper::operator SignedChangeSet_t*() {
    return this->signedChangedSet;
}

SignedChangeSetHelper::operator SignedChangeSet_t&() {
    return *this->signedChangedSet;
}

keto::asn1::ChangeSetHelperPtr SignedChangeSetHelper::getChangeSetHelper() {
    return keto::asn1::ChangeSetHelperPtr(new keto::asn1::ChangeSetHelper(&this->signedChangedSet->changeSet));
}

keto::asn1::HashHelper SignedChangeSetHelper::getHash() {
    if (!this->signedChangedSet) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::UninitializedSignedChangeSet());
    }
    return this->signedChangedSet->changeSetHash;
}


keto::asn1::SignatureHelper SignedChangeSetHelper::getSignature() {
    if (!this->signedChangedSet) {
        BOOST_THROW_EXCEPTION(keto::transaction_common::UninitializedSignedChangeSet());
    }
    return this->signedChangedSet->signature;
}

}
}

