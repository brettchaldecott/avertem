//
// Created by Brett Chaldecott on 2019/04/24.
//

#ifndef KETO_SIGNEDBLOCKWRAPPERMESSAGEPROTOHELPER_HPP
#define KETO_SIGNEDBLOCKWRAPPERMESSAGEPROTOHELPER_HPP

#include <string>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"


#include "keto/block_db/SignedBlockWrapperProtoHelper.hpp"


namespace keto {
namespace block_db {

class SignedBlockWrapperMessageProtoHelper;
typedef std::shared_ptr<SignedBlockWrapperMessageProtoHelper> SignedBlockWrapperMessageProtoHelperPtr;

class SignedBlockWrapperMessageProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    SignedBlockWrapperMessageProtoHelper(const keto::asn1::HashHelper& hash = keto::asn1::HashHelper());
    SignedBlockWrapperMessageProtoHelper(const keto::proto::SignedBlockWrapperMessage& signedBlockWrapperMessage);
    SignedBlockWrapperMessageProtoHelper(const std::string& signedBlockWrapperMessage);
    SignedBlockWrapperMessageProtoHelper(const SignedBlockWrapperMessageProtoHelper& signedBlockWrapperMessage) = default;
    virtual ~SignedBlockWrapperMessageProtoHelper();


    operator std::string();
    operator keto::proto::SignedBlockWrapperMessage();
    SignedBlockWrapperProtoHelperPtr operator[] (int index);


    SignedBlockWrapperMessageProtoHelper& addSignedBlockWrapper(
            const SignedBlockWrapperProtoHelperPtr& signedBlockWrapperProtoHelperPtr);
    SignedBlockWrapperMessageProtoHelper& addSignedBlockWrapper(
            const SignedBlockWrapperProtoHelper& signedBlockWrapperProtoHelper);
    int size() const;
    std::vector<SignedBlockWrapperProtoHelperPtr> getSignedBlockWrappers() const;
    SignedBlockWrapperProtoHelperPtr getSignedBlockWrapper(int index) const;

    SignedBlockWrapperMessageProtoHelper& setProducerEnding(bool producerEnding);
    bool getProducerEnding();

    SignedBlockWrapperMessageProtoHelper& setMessageHash(const keto::asn1::HashHelper& hash);
    keto::asn1::HashHelper getMessageHash() const;

    std::vector<keto::asn1::HashHelper> getTangles() const;
    SignedBlockWrapperMessageProtoHelper& setTangles(const std::vector<keto::asn1::HashHelper>& tangles);

private:
    keto::proto::SignedBlockWrapperMessage signedBlockWrapperMessage;
};


}
}


#endif //KETO_SIGNEDBLOCKWRAPPERMESSAGEPROTOHELPER_HPP
