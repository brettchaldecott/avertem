//
// Created by Brett Chaldecott on 2019/02/13.
//

#include "keto/transaction_common/SignedChangeSetHelper.hpp"

namespace keto {
namespace transaction_common {

std::string getSourceVersion() {
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


}
}

