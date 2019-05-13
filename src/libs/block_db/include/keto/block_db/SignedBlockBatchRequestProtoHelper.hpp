//
// Created by Brett Chaldecott on 2019-05-09.
//

#ifndef KETO_SIGNEDBLOCKBATCHREQUESTPROTOHELPER_HPP
#define KETO_SIGNEDBLOCKBATCHREQUESTPROTOHELPER_HPP

#include <string>
#include <map>
#include <memory>

#include "BlockChain.pb.h"

#include "keto/asn1/HashHelper.hpp"



namespace keto {
namespace block_db {

class SignedBlockBatchRequestProtoHelper;
typedef std::shared_ptr<SignedBlockBatchRequestProtoHelper> SignedBlockBatchRequestProtoHelperPtr;

class SignedBlockBatchRequestProtoHelper {
public:
    SignedBlockBatchRequestProtoHelper();
    SignedBlockBatchRequestProtoHelper(const keto::proto::SignedBlockBatchRequest& signedBlockBatchRequest);
    SignedBlockBatchRequestProtoHelper(const std::string& value);
    SignedBlockBatchRequestProtoHelper(const SignedBlockBatchRequestProtoHelper& orig) = default;
    virtual ~SignedBlockBatchRequestProtoHelper();


    operator keto::proto::SignedBlockBatchRequest();
    operator std::string();

    SignedBlockBatchRequestProtoHelper& addHash(const keto::asn1::HashHelper& hashHelper);
    int hashCount();
    keto::asn1::HashHelper getHash(int index);

private:
    keto::proto::SignedBlockBatchRequest signedBlockBatchRequest;
};


}
}


#endif //KETO_SIGNEDBLOCKBATCHREQUESTPROTOHELPER_HPP
