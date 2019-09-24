//
// Created by Brett Chaldecott on 2019/04/23.
//

#ifndef KETO_SIGNEDBLOCKWRAPPERPROTOHELPER_HPP
#define KETO_SIGNEDBLOCKWRAPPERPROTOHELPER_HPP

#include <string>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"
#include "BlockChainDB.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/block_db/SignedBlockBuilder.hpp"


namespace keto {
namespace block_db {

class SignedBlockWrapperProtoHelper;
typedef std::shared_ptr<SignedBlockWrapperProtoHelper> SignedBlockWrapperProtoHelperPtr;

class SignedBlockWrapperProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    SignedBlockWrapperProtoHelper(const keto::proto::BlockWrapper& blockWrapper);
    SignedBlockWrapperProtoHelper(const keto::proto::SignedBlockWrapper& wrapper);
    SignedBlockWrapperProtoHelper(const std::string& str);
    SignedBlockWrapperProtoHelper(const keto::block_db::SignedBlockBuilderPtr& signedBlockBuilderPtr);
    SignedBlockWrapperProtoHelper(const SignedBlockWrapperProtoHelper& signedBlockWrapperProtoHelper) = default;
    virtual ~SignedBlockWrapperProtoHelper();


    keto::proto::SignedBlockWrapper getSignedBlockWrapper();
    operator keto::proto::SignedBlockWrapper();

    std::vector<SignedBlockWrapperProtoHelperPtr> getNestedBlocks();
    SignedBlockWrapperProtoHelper& addNestedBlocks(
            const keto::proto::SignedBlockWrapper& wrapper);
    keto::asn1::HashHelper getHash();
    keto::asn1::HashHelper getFirstTransactionHash();
    keto::asn1::HashHelper getParentHash();

    operator SignedBlock_t&();
    operator SignedBlock_t*();

    operator std::string() const;



    SignedBlockWrapperProtoHelper& operator = (const std::string& asn1Block);

    void loadSignedBlock();
    int getHeight();
private:
    keto::proto::SignedBlockWrapper signedBlockWrapper;
    SignedBlock_t* signedBlock;


    void populateSignedBlockWrapper(keto::proto::SignedBlockWrapper& signedBlockWrapper,
                                    const keto::block_db::SignedBlockBuilderPtr& signedBlockBuilderPtr);


};


}
}


#endif //KETO_SIGNEDBLOCKWRAPPERPROTOHELPER_HPP
