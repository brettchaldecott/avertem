//
// Created by Brett Chaldecott on 2019/02/18.
//

#ifndef KETO_FEEINFOMSGPROTOHELPER_HPP
#define KETO_FEEINFOMSGPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace transaction_common {

class FeeInfoMsgProtoHelper;
typedef std::shared_ptr<FeeInfoMsgProtoHelper> FeeInfoMsgProtoHelperPtr;

class FeeInfoMsgProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();


    FeeInfoMsgProtoHelper();
    FeeInfoMsgProtoHelper(const long fee);
    FeeInfoMsgProtoHelper(const long fee, const long expiryDuration);
    FeeInfoMsgProtoHelper(const long fee, const long expiryDuration, const long maxFee);
    FeeInfoMsgProtoHelper(const keto::proto::FeeInfoMsg& feeInfoMsg);
    FeeInfoMsgProtoHelper(const std::string& msg);
    FeeInfoMsgProtoHelper(const std::vector<uint8_t>& msg);
    FeeInfoMsgProtoHelper(const FeeInfoMsgProtoHelper& orig) = default;
    virtual ~FeeInfoMsgProtoHelper();


    FeeInfoMsgProtoHelper& setFeeRatio(const long fee);
    long getFeeRatio();

    FeeInfoMsgProtoHelper& setMaxFee(const long maxFee);
    long getMaxFee();

    std::time_t getExpiryTime();
    FeeInfoMsgProtoHelper& setExpiryTime(const std::time_t& expiry);
    bool isExpired();

    operator keto::proto::FeeInfoMsg();
    operator std::string();



private:
    keto::proto::FeeInfoMsg feeInfoMsg;
};

}
}


#endif //KETO_FEEINFOMSGPROTOHELPER_HPP
