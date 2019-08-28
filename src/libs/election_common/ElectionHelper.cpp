//
// Created by Brett Chaldecott on 2019-08-21.
//

#include "keto/election_common/ElectionHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"

namespace keto {
namespace election_common {


std::string ElectionHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionHelper::ElectionHelper() {
    this->election = (Election_t*)calloc(1, sizeof *election);
    this->setDate(keto::asn1::TimeHelper());
}

ElectionHelper::ElectionHelper(const Election_t* election) {
    this->election = keto::asn1::clone<Election_t>(election,&asn_DEF_Election);
}

ElectionHelper::ElectionHelper(const Election_t& election) {
    this->election = keto::asn1::clone<Election_t>(&election,&asn_DEF_Election);
}

ElectionHelper::ElectionHelper(const ElectionHelper& electionHelper) {
    this->election = keto::asn1::clone<Election_t>(electionHelper.election,&asn_DEF_Election);
}

ElectionHelper::~ElectionHelper() {
    if (this->election) {
        ASN_STRUCT_FREE(asn_DEF_Election, election);
        election = NULL;
    }
}

long ElectionHelper::getVersion() {
    return this->election->version;
}

keto::asn1::TimeHelper ElectionHelper::getDate() {
    return this->election->date;
}

ElectionHelper& ElectionHelper::setDate(const keto::asn1::TimeHelper& timeHelper) {
    this->election->date = timeHelper;
    return *this;
}

keto::asn1::HashHelper ElectionHelper::getAccountHash() {
    return this->election->accountHash;
}

ElectionHelper& ElectionHelper::setAccountHash(const keto::asn1::HashHelper& accountHash) {
    this->election->accountHash = accountHash;
    return *this;
}

keto::software_consensus::SoftwareConsensusHelper ElectionHelper::getAcceptedCheck() {
    return this->election->acceptedCheck;
}

ElectionHelper& ElectionHelper::setAcceptedCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper) {
    this->election->acceptedCheck = softwareConsensusHelper;
    return *this;
}

keto::software_consensus::SoftwareConsensusHelper ElectionHelper::getValidateCheck() {
    return this->election->acceptedCheck;
}

ElectionHelper& ElectionHelper::setValidateCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper) {
    this->election->acceptedCheck = softwareConsensusHelper;
    return *this;
}

ElectionHelper::operator Election_t*() {
    return keto::asn1::clone<Election_t>(election,&asn_DEF_Election);
}

ElectionHelper::operator Election_t() const {
    Election_t* electionPtr = keto::asn1::clone<Election_t>(election,&asn_DEF_Election);
    Election_t election = *electionPtr;
    free(electionPtr);
    return election;
}

Election_t* ElectionHelper::getElection() {
    return keto::asn1::clone<Election_t>(election,&asn_DEF_Election);
}


ElectionHelper::operator std::vector<uint8_t>() {
    return keto::asn1::SerializationHelper<Election>(this->election,&asn_DEF_Election);
}

ElectionHelper::operator keto::crypto::SecureVector() {
    return keto::asn1::SerializationHelper<Election>(this->election,&asn_DEF_Election);
}

}
}