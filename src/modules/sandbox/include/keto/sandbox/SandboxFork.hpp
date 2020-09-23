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

        keto::event::Event executeActionMessage(const keto::event::Event& event);
        keto::event::Event executeHttpActionMessage(const keto::event::Event& event);
    private:
    };

    class Parent {
    public:

        Parent();
        Parent(const Parent& orig) = delete;
        virtual ~Parent();

        keto::event::Event executeActionMessage(const keto::event::Event& event);
        keto::event::Event executeHttpActionMessage(const keto::event::Event& event);
    private:
        keto::wavm_common::PipePtr inPipe;
        keto::wavm_common::PipePtr outPipe;
        std::shared_ptr<boost::process::child> childPtr;

        keto::event::Event execute();
        void write(boost::process::opstream& pout, const keto::wavm_common::ForkMessageWrapperHelper& forkMessageWrapperHelper);
        keto::wavm_common::ForkMessageWrapperHelper read(boost::process::ipstream& pin);
    };

    SandboxFork(const keto::event::Event& event);
    SandboxFork(const SandboxFork& orig) = delete;
    virtual ~SandboxFork();

    keto::event::Event executeActionMessage();
    keto::event::Event executeHttpActionMessage();
private:
    keto::event::Event event;


};


}
}


#endif //KETO_SANDBOXFORK_HPP
