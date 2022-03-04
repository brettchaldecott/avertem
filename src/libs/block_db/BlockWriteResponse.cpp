//
// Created by Brett Chaldecott on 2022/01/20.
//

#include "keto/block_db/BlockWriteResponse.hpp"

namespace keto {
namespace block_db {

std::string BlockWriteResponse::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

BlockWriteResponse::BlockWriteResponse() : success(true){

}

BlockWriteResponse::~BlockWriteResponse() {

}

bool BlockWriteResponse::isSuccess() {
    return success;
}


SignedBlockBatchRequestProtoHelperPtr BlockWriteResponse::getSignedBlockBatchRequest() {
    return signedBlockBatchRequestProtoHelperPtr;
}

void BlockWriteResponse::addAccountSyncHash(const keto::asn1::HashHelper& hashHelper) {
    this->success = false;
    if (!signedBlockBatchRequestProtoHelperPtr) {
        signedBlockBatchRequestProtoHelperPtr = SignedBlockBatchRequestProtoHelperPtr(new SignedBlockBatchRequestProtoHelper());
    }
    signedBlockBatchRequestProtoHelperPtr->addHash(hashHelper);
}

void BlockWriteResponse::addSignedBlockBatchRequest(const SignedBlockBatchRequestProtoHelperPtr& signedBlockBatchRequestProtoHelperPtr) {
    for (int index = 0; index < signedBlockBatchRequestProtoHelperPtr->hashCount(); index++) {
        this->addAccountSyncHash(signedBlockBatchRequestProtoHelperPtr->getHash(index));
    }
}


}
}