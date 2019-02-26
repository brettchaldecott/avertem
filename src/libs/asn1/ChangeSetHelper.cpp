/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ChangeSetHelper.cpp
 * Author: ubuntu
 * 
 * Created on March 13, 2018, 10:54 AM
 */

#include <ChangeSet.h>
#include "ChangeData.h"

#include "keto/asn1/ChangeSetHelper.hpp"
#include "keto/asn1/Exception.hpp"
#include "keto/asn1/CloneHelper.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace asn1 {

std::string ChangeSetHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ChangeSetHelper::ChangeSetHelper() {
}

ChangeSetHelper::ChangeSetHelper(ChangeSet_t* changeSet) : own(false), changeSet(changeSet) {
}

ChangeSetHelper::ChangeSetHelper(const HashHelper& transactionHash,
        const HashHelper& accountHash) : own(true), transactionHash(transactionHash),
    accountHash(accountHash) {
    changeSet = (ChangeSet_t*)calloc(1, sizeof *changeSet);
    changeSet->version = keto::common::MetaInfo::PROTOCOL_VERSION;
    changeSet->transactionHash = this->transactionHash;
    changeSet->accountHash = this->accountHash;
}

ChangeSetHelper::~ChangeSetHelper() {
    if (own && this->changeSet) {
        ASN_STRUCT_FREE(asn_DEF_ChangeSet, this->changeSet);
    }
    this->changeSet = NULL;
}

ChangeSetHelper& ChangeSetHelper::setTransactionHash(const HashHelper& transactionHash) {
    changeSet->transactionHash = transactionHash;
    return (*this);
}

ChangeSetHelper& ChangeSetHelper::setAccountHash(const HashHelper& accountHash) {
    changeSet->accountHash = accountHash;
    return (*this);
}

ChangeSetHelper& ChangeSetHelper::setStatus(const Status_t status) {
    changeSet->status = status;
    return (*this);
}

Status_t ChangeSetHelper::getStatus() {
    return this->changeSet->status;
}

ChangeSetHelper& ChangeSetHelper::addChange(const ANY_t& change) {
    ChangeData_t* changeData = (ChangeData_t*)calloc(1, sizeof *changeSet);
    changeData->choice.asn1Change = change;
    changeData->present = ChangeData_PR_asn1Change;
    if (0!= ASN_SEQUENCE_ADD(&changeSet->changes,changeData)) {
        ASN_STRUCT_FREE(asn_DEF_ChangeSet, changeSet);
        BOOST_THROW_EXCEPTION(keto::asn1::FailedToAddChangeToChangeSetException());
    }
    return (*this);
}

std::vector<ChangeSetDataHelperPtr> ChangeSetHelper::getChanges() {
    std::vector<ChangeSetDataHelperPtr> result;
    for (int index = 0; index < this->changeSet->changes.list.count; index++) {
        result.push_back(ChangeSetDataHelperPtr(new ChangeSetDataHelper(
                this->changeSet->changes.list.array[index])));
    }
    return result;
}

ChangeSetHelper::operator ChangeSet_t*() {
    return keto::asn1::clone<ChangeSet_t>(this->changeSet, &asn_DEF_ChangeSet);
}

ChangeSetHelper::operator ANY_t*() {
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_ChangeSet, this->changeSet);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
    }
    return anyPtr;
}



}
}