//
// Created by Brett Chaldecott on 2019-09-24.
//

#include "keto/chain_query_common/ProducerInfoResultProtoHelper.hpp"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace chain_query_common {

std::string ProducerInfoResultProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ProducerInfoResultProtoHelper::ProducerInfoResultProtoHelper() {
    this->producerInfoResult.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ProducerInfoResultProtoHelper::ProducerInfoResultProtoHelper(const keto::proto::ProducerInfoResult& producerInfoResult)
    : producerInfoResult(producerInfoResult) {

}

ProducerInfoResultProtoHelper::ProducerInfoResultProtoHelper(const std::string& msg) {
    this->producerInfoResult.ParseFromString(msg);
}

ProducerInfoResultProtoHelper::~ProducerInfoResultProtoHelper() {

}

keto::asn1::HashHelper ProducerInfoResultProtoHelper::getAccountHashId() {
    return this->producerInfoResult.account_hash_id();
}

ProducerInfoResultProtoHelper& ProducerInfoResultProtoHelper::setAccountHashId(const keto::asn1::HashHelper& hashHelper) {
    this->producerInfoResult.set_account_hash_id(hashHelper);
    return *this;
}

std::vector<keto::asn1::HashHelper> ProducerInfoResultProtoHelper::getTangles() {
    std::vector<keto::asn1::HashHelper> result;
    for (int index = 0; index < this->producerInfoResult.tangles_size(); index++) {
        result.push_back(this->producerInfoResult.tangles(index));
    }
    return result;
}

ProducerInfoResultProtoHelper& ProducerInfoResultProtoHelper::setTangles(const std::vector<keto::asn1::HashHelper>& tangles) {
    for (keto::asn1::HashHelper tangle : tangles) {
        addTangle(tangle);
    }
    return *this;
}

ProducerInfoResultProtoHelper& ProducerInfoResultProtoHelper::addTangle(const keto::asn1::HashHelper& tangle) {
    this->producerInfoResult.add_tangles(tangle);
    return *this;
}

ProducerInfoResultProtoHelper::operator keto::proto::ProducerInfoResult() const {
    return this->producerInfoResult;
}

ProducerInfoResultProtoHelper::operator std::string() const {
    return this->producerInfoResult.SerializeAsString();
}


}
}