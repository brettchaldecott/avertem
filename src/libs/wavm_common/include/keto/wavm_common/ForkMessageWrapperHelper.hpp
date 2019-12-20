//
// Created by Brett Chaldecott on 2019/12/18.
//

#ifndef KETO_FORKMESSAGEWRAPPERHELPER_HPP
#define KETO_FORKMESSAGEWRAPPERHELPER_HPP

#include <string>
#include <memory>

#include "Protocol.pb.h"

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace wavm_common {

class ForkMessageWrapperHelper;
typedef std::shared_ptr<ForkMessageWrapperHelper> ForkMessageWrapperHelperPtr;

class ForkMessageWrapperHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    ForkMessageWrapperHelper();
    ForkMessageWrapperHelper(const std::string& message);
    ForkMessageWrapperHelper(const std::vector<uint8_t>& message);
    ForkMessageWrapperHelper(const keto::proto::ForkMessageWrapper& forkMessageWrapper);
    ForkMessageWrapperHelper(const ForkMessageWrapperHelper& orig) = default;
    virtual ~ForkMessageWrapperHelper();

    ForkMessageWrapperHelper& setCommand(const std::string& command);
    std::string getCommand();

    ForkMessageWrapperHelper& setEvent(const std::string& event);
    std::string getEvent();

    ForkMessageWrapperHelper& setMessage(const std::string& message);
    ForkMessageWrapperHelper& setMessage(const std::vector<uint8_t>& message);
    std::string getMessage();

    ForkMessageWrapperHelper& setException(const std::string& exception);
    std::string getException();

    ForkMessageWrapperHelper& operator = (const std::string& message);
    ForkMessageWrapperHelper& operator = (const keto::proto::ForkMessageWrapper& forkMessageWrapper);
    operator keto::proto::ForkMessageWrapper() const;
    operator std::string() const;
    operator std::vector<uint8_t>() const;

private:
    keto::proto::ForkMessageWrapper forkMessageWrapper;

};


}
}



#endif //KETO_FORKMESSAGEWRAPPERHELPER_HPP
