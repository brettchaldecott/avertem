//
// Created by Brett Chaldecott on 2019-09-24.
//


#include "keto/chain_query_common/ProducerResultProtoHelper.hpp"

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace chain_query_common {


std::string ProducerResultProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ProducerResultProtoHelper::ProducerResultProtoHelper() {
    this->producerResult.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
}

ProducerResultProtoHelper::ProducerResultProtoHelper(const keto::proto::ProducerResult& producerResult) :
    producerResult(producerResult) {

}

ProducerResultProtoHelper::ProducerResultProtoHelper(const std::string& msg) {
    this->producerResult.ParseFromString(msg);
}

ProducerResultProtoHelper::~ProducerResultProtoHelper() {
}

std::vector<ProducerInfoResultProtoHelperPtr> ProducerResultProtoHelper::getProducers() {
    std::vector<ProducerInfoResultProtoHelperPtr> result;
    for (int index = 0; index < this->producerResult.tangles_size(); index++) {
        result.push_back(ProducerInfoResultProtoHelperPtr(
                new ProducerInfoResultProtoHelper(this->producerResult.tangles(index))));
    }
    return result;
}

ProducerResultProtoHelper& ProducerResultProtoHelper::addProducer(const ProducerInfoResultProtoHelper& producer) {
    *this->producerResult.add_tangles() = producer;
    return *this;
}

ProducerResultProtoHelper& ProducerResultProtoHelper::addProducer(const ProducerInfoResultProtoHelperPtr& producer) {
    *this->producerResult.add_tangles() = *producer;
    return *this;
}

ProducerResultProtoHelper::operator keto::proto::ProducerResult() const {
    return this->producerResult;
}

ProducerResultProtoHelper::operator std::string() const {
    return this->producerResult.SerializeAsString();
}



}
}