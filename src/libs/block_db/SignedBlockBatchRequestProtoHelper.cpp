//
// Created by Brett Chaldecott on 2019-05-09.
//

#include "keto/block_db/SignedBlockBatchRequestProtoHelper.hpp"


namespace keto {
namespace block_db {

SignedBlockBatchRequestProtoHelper::SignedBlockBatchRequestProtoHelper() {

}


SignedBlockBatchRequestProtoHelper::SignedBlockBatchRequestProtoHelper(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest)  :
        signedBlockBatchRequest(signedBlockBatchRequest) {
}


SignedBlockBatchRequestProtoHelper::SignedBlockBatchRequestProtoHelper(const std::string& value) {
    this->signedBlockBatchRequest.ParseFromString(value);
}

SignedBlockBatchRequestProtoHelper::~SignedBlockBatchRequestProtoHelper() {
}


SignedBlockBatchRequestProtoHelper::operator keto::proto::SignedBlockBatchRequest() {
    return this->signedBlockBatchRequest;
}

SignedBlockBatchRequestProtoHelper::operator std::string() {
    std::string result;
    this->signedBlockBatchRequest.SerializePartialToString(&result);
    return result;
}


SignedBlockBatchRequestProtoHelper& SignedBlockBatchRequestProtoHelper::addHash(const keto::asn1::HashHelper& hashHelper) {
    this->signedBlockBatchRequest.add_tangle_hashes(hashHelper);
    return *this;
}

int SignedBlockBatchRequestProtoHelper::hashCount() {
    return this->signedBlockBatchRequest.tangle_hashes_size();
}

keto::asn1::HashHelper SignedBlockBatchRequestProtoHelper::getHash(int index) {
    if (index >= this->signedBlockBatchRequest.tangle_hashes_size()) {
        return keto::asn1::HashHelper();
    }
    return keto::asn1::HashHelper(this->signedBlockBatchRequest.tangle_hashes(index));
}


}
}