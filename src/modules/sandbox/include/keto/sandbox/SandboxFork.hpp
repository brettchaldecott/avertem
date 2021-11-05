//
// Created by Brett Chaldecott on 2019/12/17.
//

#ifndef KETO_SANDBOXFORK_HPP
#define KETO_SANDBOXFORK_HPP

#include <boost/process/pipe.hpp>
#include <boost/process/child.hpp>

#include <string>
#include <memory>

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/wavm_common/ParentForkGateway.hpp"

namespace keto {
namespace sandbox {

class SandboxFork;
typedef std::shared_ptr<SandboxFork> SandboxForkPtr;
typedef std::shared_ptr<boost::process::ipstream> IpStreamPipePtr;
typedef std::shared_ptr<boost::process::opstream> OpStreamPipePtr;

class SandboxFork {
public:
    static const std::string PING;
    static const std::string PONG;
    static const std::string TERMINATE;
    static const std::string EXECUTE_ACTION;
    static const std::string EXECUTE_ACTION_MESSAGE;
    static const std::string EXECUTE_HTTP;
    static const std::string EXECUTE_HTTP_MESSAGE;


    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class Child {
    public:
        static const long CHILD_STACK_SIZE;

        Child(const keto::wavm_common::PipePtr& inPipe, const keto::wavm_common::PipePtr& outPipe);
        Child(const Child& orig) = delete;
        virtual ~Child();

        void execute();
    private:
        keto::event::Event executeActionMessage(const keto::event::Event& event);
        keto::event::Event executeHttpActionMessage(const keto::event::Event& event);
    };

    class ParentStream {
    public:
        ParentStream(keto::wavm_common::PipePtr inPipe, keto::wavm_common::PipePtr outPipe);
        ParentStream(const ParentStream& orig) = delete;
        virtual ~ParentStream();

        boost::process::ipstream& getPin();
        boost::process::opstream& getPout();
    private:
        boost::process::ipstream pin;
        boost::process::opstream pout;
    };
    typedef std::shared_ptr<ParentStream> ParentStreamPtr;


    class Parent {
    public:

        Parent();
        Parent(const Parent& orig) = delete;
        virtual ~Parent();

        keto::event::Event executeActionMessage(const keto::event::Event& event);
        keto::event::Event executeHttpActionMessage(const keto::event::Event& event);
        bool terminate();
    private:
        keto::wavm_common::PipePtr inPipe;
        keto::wavm_common::PipePtr outPipe;
        std::shared_ptr<boost::process::child> childPtr;
        ParentStreamPtr parentStream;

        bool validateFork(boost::process::ipstream& pin, boost::process::opstream& pout);
        keto::event::Event execute(boost::process::ipstream& pin, boost::process::opstream& pout);
        void write(boost::process::opstream& pout, const keto::wavm_common::ForkMessageWrapperHelper& forkMessageWrapperHelper);
        void write(boost::process::opstream& pout, const std::string message);
        keto::wavm_common::ForkMessageWrapperHelper read(boost::process::ipstream& pin);
        std::shared_ptr<std::string> readText(boost::process::ipstream& pin);
    };


    SandboxFork();
    SandboxFork(const SandboxFork& orig) = delete;
    virtual ~SandboxFork();

    int getUsageCount();
    int incrementUsageCount();
    keto::event::Event executeActionMessage(const keto::event::Event& event);
    keto::event::Event executeHttpActionMessage(const keto::event::Event& event);
    bool terminate();
private:
    std::mutex classMutex;
    int usageCount;
    Parent parent;


};


}
}


#endif //KETO_SANDBOXFORK_HPP
