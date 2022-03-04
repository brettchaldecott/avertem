//
// Created by Brett Chaldecott on 2022/01/20.
//

#ifndef KETO_BLOCKWRITERESPONSE_H
#define KETO_BLOCKWRITERESPONSE_H

#include <string>
#include <memory>


#include "keto/obfuscate/MetaString.hpp"

#include "keto/block_db/SignedBlockBatchRequestProtoHelper.hpp"

namespace keto {
namespace block_db {

class BlockWriteResponse;
typedef std::shared_ptr<BlockWriteResponse> BlockWriteResponsePtr;

class BlockWriteResponse {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    BlockWriteResponse();
    BlockWriteResponse(const BlockWriteResponse& orig) = delete;
    virtual ~BlockWriteResponse();

    bool isSuccess();

    SignedBlockBatchRequestProtoHelperPtr getSignedBlockBatchRequest();
    void addAccountSyncHash(const keto::asn1::HashHelper& hashHelper);
    void addSignedBlockBatchRequest(const SignedBlockBatchRequestProtoHelperPtr& signedBlockBatchRequestProtoHelperPtr);

private:

    bool success;
    SignedBlockBatchRequestProtoHelperPtr signedBlockBatchRequestProtoHelperPtr;

};
}
}


#endif //KETO_BLOCKWRITERESPONSE_H
