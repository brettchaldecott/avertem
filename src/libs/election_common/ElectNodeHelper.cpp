//
// Created by Brett Chaldecott on 2019-08-26.
//

#include "keto/election_common/ElectNodeHelper.hpp"
#include "keto/election_common/Exception.hpp"


namespace keto {
namespace election_common {

std::string ElectNodeHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectNodeHelper::ElectNodeHelper() {
    this->electNode = (ElectNode_t*)calloc(1, sizeof *electNode);
    this->setDate(keto::asn1::TimeHelper());
}

ElectNodeHelper::ElectNodeHelper(const ElectNode_t* electNode) {
    this->electNode = keto::asn1::clone<ElectNode_t>(electNode,&asn_DEF_ElectNode);
}

ElectNodeHelper::ElectNodeHelper(const ElectNode_t& electNode) {
    this->electNode = keto::asn1::clone<ElectNode_t>(&electNode,&asn_DEF_ElectNode);
}

ElectNodeHelper::ElectNodeHelper(const ElectNodeHelper& electNodeHelper) {
    this->electNode = keto::asn1::clone<ElectNode_t>(electNodeHelper.electNode,&asn_DEF_Election);
}

ElectNodeHelper::~ElectNodeHelper() {
    if (this->electNode) {
        ASN_STRUCT_FREE(asn_DEF_ElectNode, electNode);
        electNode = NULL;
    }
}

long ElectNodeHelper::getVersion() {
    return this->electNode->version;
}

keto::asn1::TimeHelper ElectNodeHelper::getDate() {
    return this->electNode->date;
}

ElectNodeHelper& ElectNodeHelper::setDate(const keto::asn1::TimeHelper& timeHelper) {
    this->electNode->date = timeHelper;
    return *this;
}

SignedElectionHelperPtr ElectNodeHelper::getElectedNode() {
    return SignedElectionHelperPtr(new SignedElectionHelper(this->electNode->electedNode));
}

ElectNodeHelper& ElectNodeHelper::setElectedNode(const SignedElectionHelper& signedElectionHelper) {
    this->electNode->electedNode = signedElectionHelper;
    return *this;
}

std::vector<SignedElectionHelperPtr> ElectNodeHelper::getAlternatives() {
    std::vector<SignedElectionHelperPtr> result;
    for (int index = 0; index < this->electNode->alternatives.list.count; index++) {
        result.push_back(SignedElectionHelperPtr(new SignedElectionHelper(this->electNode->alternatives.list.array[index])));
    }
    return result;
}

ElectNodeHelper& ElectNodeHelper::addAlternative(const SignedElectionHelper& signedElectionHelper) {
    if (0 != ASN_SEQUENCE_ADD(&this->electNode->alternatives,signedElectionHelper)) {
        BOOST_THROW_EXCEPTION(keto::election_common::FailedToAddAlternativeException());
    }
    return *this;
}

keto::software_consensus::SoftwareConsensusHelper ElectNodeHelper::getAcceptedCheck() {
    return this->electNode->acceptedCheck;
}

ElectNodeHelper& ElectNodeHelper::setAcceptedCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper) {
    this->electNode->acceptedCheck = softwareConsensusHelper;
    return *this;
}

keto::software_consensus::SoftwareConsensusHelper ElectNodeHelper::getValidateCheck() {
    return this->electNode->validateCheck;
}

ElectNodeHelper& ElectNodeHelper::setValidateCheck(const keto::software_consensus::SoftwareConsensusHelper& softwareConsensusHelper) {
    this->electNode->validateCheck = softwareConsensusHelper;
    return *this;
}


ElectNodeHelper::operator ElectNode_t*() {
    return keto::asn1::clone<ElectNode_t>(this->electNode,&asn_DEF_ElectNode);
}

ElectNodeHelper::operator ElectNode_t() const {
    ElectNode_t* electNodePtr = keto::asn1::clone<ElectNode_t>(electNode,&asn_DEF_ElectNode);
    ElectNode_t electNode = *electNodePtr;
    free(electNodePtr);
    return electNode;
}

ElectNode_t* ElectNodeHelper::getElection() {
    return keto::asn1::clone<ElectNode_t>(this->electNode,&asn_DEF_ElectNode);
}

ElectNodeHelper::operator std::vector<uint8_t>() {
    return keto::asn1::SerializationHelper<ElectNode_t>(this->electNode,&asn_DEF_ElectNode);
}

ElectNodeHelper::operator keto::crypto::SecureVector() {
    return keto::asn1::SerializationHelper<ElectNode_t>(this->electNode,&asn_DEF_ElectNode);
}


}
}