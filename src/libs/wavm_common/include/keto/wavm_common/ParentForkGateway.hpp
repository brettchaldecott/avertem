//
// Created by Brett Chaldecott on 2019/12/17.
//

#ifndef KETO_PARENTFORKGATEWAY_HPP
#define KETO_PARENTFORKGATEWAY_HPP

#include <boost/process/pipe.hpp>
#include <boost/process/child.hpp>

#include <string>
#include <memory>


#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/wavm_common/ForkMessageWrapperHelper.hpp"

namespace keto {
namespace wavm_common {

class ParentForkGateway;
typedef std::shared_ptr<ParentForkGateway> ParentForkGatewayPtr;
typedef std::shared_ptr<boost::process::pipe> PipePtr;

class ParentForkGateway {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class REQUEST {
    public:
        static const char* RAISE_EXCEPTION;
        static const char* PROCESS_EVENT;
        static const char* TRIGGER_EVENT;
        static const char* RETURN_RESULT;
        static const std::string EXECUTE_CONFIRM;
        static const std::string MESSAGE_CONFIRM;
    };

    ParentForkGateway(const PipePtr& inPipe, const PipePtr& outPipe);
    ParentForkGateway(const ParentForkGateway& orig) = delete;
    virtual ~ParentForkGateway();

    static ParentForkGatewayPtr init(const PipePtr& inPipe, const PipePtr& outPipe);
    static void fin();

    static void raiseException(const std::string& exception, bool throwEx = true);
    static keto::event::Event processEvent(const keto::event::Event& event);
    static void triggerEvent(const keto::event::Event& event);
    static void returnResult(const keto::event::Event& event);
    static void ping();
    static std::shared_ptr<std::string> getCommand();
    static keto::event::Event getRequest();

private:
    boost::process::ipstream pin;
    boost::process::opstream pout;

    static ParentForkGatewayPtr getInstance();

    void _raiseException(const std::string& exception, bool throwEx = true);
    keto::event::Event _processEvent(const keto::event::Event& event);
    void _triggerEvent(const keto::event::Event& event);
    void _returnResult(const keto::event::Event& event);
    void _ping();
    std::shared_ptr<std::string> _getCommand();
    keto::event::Event _getRequest();

    keto::wavm_common::ForkMessageWrapperHelper read();
    std::shared_ptr<std::string> readText();
    void write(const std::string& value);
    void write(const keto::wavm_common::ForkMessageWrapperHelper& forkMessageWrapperHelper);
};


}
}



#endif //KETO_PARENTFORKGATEWAY_HPP
