//
// Created by Brett Chaldecott on 2019/02/13.
//

#include "keto/transaction_common/SignedChangeSetHelper.hpp"
#include "../../../ide_build/src/protocol/asn1/SignedChangeSet.h"

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
    if (this->signedChangedSet) {

    }
    return this->signedChangedSet->changeSetHash;
}


}
}

