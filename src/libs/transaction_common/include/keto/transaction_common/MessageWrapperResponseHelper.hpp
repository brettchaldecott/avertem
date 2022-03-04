//
// Created by Brett Chaldecott on 2022/02/23.
//

#ifndef KETO_MESSAGEWRAPPERRESPONSEHELPER_H
#define KETO_MESSAGEWRAPPERRESPONSEHELPER_H

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Protocol.pb.h"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace transaction_common {

class MessageWrapperResponseHelper;
typedef std::shared_ptr<MessageWrapperResponseHelper> MessageWrapperResponseHelperPtr;


class MessageWrapperResponseHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    MessageWrapperResponseHelper();
    MessageWrapperResponseHelper(const keto::proto::MessageWrapperResponse& messageWrapperResponse);
    MessageWrapperResponseHelper(const std::string& msg);
    MessageWrapperResponseHelper(const MessageWrapperResponseHelper& messageWrapperResponseHelper) = default;
    virtual ~MessageWrapperResponseHelper();

    bool isSuccess();
    MessageWrapperResponseHelper& setSuccess(bool success);

    std::string getResult();
    MessageWrapperResponseHelper& setResult(const std::string& result);

    std::string getMsg();
    std::string getBinaryMsg();
    MessageWrapperResponseHelper& setMsg(const std::string& msg);
    MessageWrapperResponseHelper& setBinaryMsg(const std::string& binaryMsg);

    operator std::string();
    operator keto::proto::MessageWrapperResponse();
    keto::proto::MessageWrapperResponse getMessageWrapperResponse();


private:
    keto::proto::MessageWrapperResponse messageWrapperResponse;
};

}
}

#endif //KETO_MESSAGEWRAPPERRESPONSEHELPER_H
