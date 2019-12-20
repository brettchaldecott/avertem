//
// Created by Brett Chaldecott on 2019/12/17.
//

#include "keto/sandbox/SandboxFork.hpp"
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <sched.h>
#include <errno.h>
#include <sys/mman.h>

#include <condition_variable>

#include <botan/hex.h>

#include "Sandbox.pb.h"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"

#include "keto/sandbox/SandboxService.hpp"
#include "keto/sandbox/Exception.hpp"

#include "keto/wavm_common/WavmEngineManager.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"
#include "keto/wavm_common/WavmSessionScope.hpp"
#include "keto/wavm_common/WavmSessionTransaction.hpp"
#include "keto/wavm_common/WavmSessionHttp.hpp"
#include "keto/wavm_common/ParentForkGateway.hpp"
#include "keto/wavm_common/ForkMessageWrapperHelper.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace sandbox {

std::string SandboxFork::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

struct CloneArgument {
    keto::wavm_common::PipePtr inPtr;
    keto::wavm_common::PipePtr outPtr;
    keto::event::Event event;
};

static int _executeActionMessage(void* args) {
    std::cout << "[_executeActionMessage] Execution process" << std::endl;
    CloneArgument* argument = (CloneArgument*)args;
    std::cout << "[_executeActionMessage] execute the action" << std::endl;
    SandboxFork::Child(argument->inPtr,argument->outPtr).executeActionMessage(argument->event);
    std::cout << "[_executeActionMessage] Return from the process" << std::endl;
    return 0;
}

static int _executeHttpActionMessage(void* args) {
    std::cout << "[_executeActionMessage] Execution http process process" << std::endl;
    CloneArgument* argument = (CloneArgument*)args;
    std::cout << "[_executeActionMessage] execute the http action" << std::endl;
    SandboxFork::Child(argument->inPtr,argument->outPtr).executeHttpActionMessage(argument->event);
    std::cout << "[_executeActionMessage] return from the process" << std::endl;
    return 0;
}

const long SandboxFork::Child::CHILD_STACK_SIZE = (1024 * 1024);

SandboxFork::Child::Child(const keto::wavm_common::PipePtr& inPipe, const keto::wavm_common::PipePtr& outPipe) {
    keto::wavm_common::ParentForkGateway::init(inPipe,outPipe);
}

SandboxFork::Child::~Child() {
    // exit this method
    //KETO_LOG_ERROR << "[SandboxFork::Child::~Child] Sandbox fork child exit";
    keto::wavm_common::ParentForkGateway::fin();
    //KETO_LOG_ERROR << "[SandboxFork::Child::~Child] Exit the process";
    //std::quick_exit(0);
    //exit(0);
}

keto::event::Event SandboxFork::Child::executeActionMessage(const keto::event::Event& event) {
    try {
        keto::proto::SandboxCommandMessage sandboxCommandMessage =
                keto::server_common::fromEvent<keto::proto::SandboxCommandMessage>(event);
        keto::wavm_common::WavmSessionScope wavmSessionScope(sandboxCommandMessage);
        std::string buffer = sandboxCommandMessage.contract();
        std::string code = keto::server_common::VectorUtils().copyVectorToString(Botan::hex_decode(
                buffer,true));
        keto::wavm_common::WavmEngineManager::getInstance()->getEngine(code,sandboxCommandMessage.contract_name())->execute();
        sandboxCommandMessage = std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransaction>(wavmSessionScope.getSession())->getSandboxCommandMessage();
        keto::wavm_common::ParentForkGateway::returnResult(
                keto::server_common::toEvent<keto::proto::SandboxCommandMessage>(sandboxCommandMessage));
    } catch (keto::common::Exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        ss << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    } catch (boost::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeActionMessage]Failed to process the contract : " << boost::diagnostic_information(ex,true) << std::endl;
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    } catch (std::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    } catch (...) {
        std::stringstream ss;
        ss << "[[SandboxFork::Child::executeActionMessage]]Failed to process the contract" << std::endl;
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    }
    return keto::event::Event();
}

keto::event::Event SandboxFork::Child::executeHttpActionMessage(const keto::event::Event& event) {
    try {
        keto::proto::HttpRequestMessage httpRequestMessage =
                keto::server_common::fromEvent<keto::proto::HttpRequestMessage>(event);
        keto::wavm_common::WavmSessionScope wavmSessionScope(httpRequestMessage);
        std::string buffer = httpRequestMessage.contract();
        std::string code = keto::server_common::VectorUtils().copyVectorToString(Botan::hex_decode(
                buffer,true));
        keto::wavm_common::WavmEngineManager::getInstance()->getEngine(code,httpRequestMessage.contract_name())->executeHttp();
        keto::wavm_common::ParentForkGateway::returnResult(keto::server_common::toEvent<keto::proto::HttpResponseMessage>(
                std::dynamic_pointer_cast<keto::wavm_common::WavmSessionHttp>(wavmSessionScope.getSession())->getHttpResponse()));
    } catch (keto::common::Exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        ss << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    } catch (boost::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    } catch (std::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    } catch (...) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract" << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str());
    }
    return keto::event::Event();
}


SandboxFork::Parent::Parent()
    : inPipe(new boost::process::pipe()), outPipe(new boost::process::pipe()), stack(NULL) {
    //this->stack = (char*)mmap(NULL, SandboxFork::Child::CHILD_STACK_SIZE, PROT_READ | PROT_WRITE,
    //             MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    //if (this->stack == MAP_FAILED) {
    //    BOOST_THROW_EXCEPTION(ChildStackCreationFailedException());
    //}
}

SandboxFork::Parent::~Parent() {
    try {
        if (childPtr) {
            //childPtr->terminate();
            childPtr->wait();
            childPtr.reset();
        }
        //munmap(stack, SandboxFork::Child::CHILD_STACK_SIZE);
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Failed to process the contract : " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract";
    }
}

keto::event::Event SandboxFork::Parent::executeActionMessage(const keto::event::Event& event) {
    /*CloneArgument cloneArgument;
    cloneArgument.outPtr = this->outPipe;
    cloneArgument.inPtr = this->inPipe;
    cloneArgument.event = event;

    char *stackTop = this->stack + SandboxFork::Child::CHILD_STACK_SIZE;
    int pid = clone(_executeActionMessage,stackTop,CLONE_NEWUTS | SIGCHLD,&cloneArgument);
    if (pid == -1) {
        int errsv = errno;
        std::string error = strerror(errsv);
        std::stringstream ss;
        ss << "Failed to clone : " << error;
        BOOST_THROW_EXCEPTION(CloneFailedException(ss.str()));
    }
    this->childPtr = std::make_shared<boost::process::child>(pid);*/
    int pid = fork();
    if (pid == 0) {
        try {
            SandboxFork::Child(this->inPipe, this->outPipe).executeActionMessage(event);
            std::quick_exit(0);
        } catch (...) {
            std::quick_exit(-1);
        }
        return keto::event::Event();
    }
    this->childPtr = std::make_shared<boost::process::child>(pid);
    return execute();
}

keto::event::Event SandboxFork::Parent::executeHttpActionMessage(const keto::event::Event& event) {
    /*CloneArgument cloneArgument;
    cloneArgument.outPtr = this->outPipe;
    cloneArgument.inPtr = this->inPipe;
    cloneArgument.event = event;

    char *stackTop = this->stack + SandboxFork::Child::CHILD_STACK_SIZE;
    int pid = clone(_executeHttpActionMessage,stackTop,CLONE_NEWUTS | SIGCHLD,&cloneArgument);
    if (pid == -1) {
        int errsv = errno;
        std::string error = strerror(errsv);
        std::stringstream ss;
        ss << "Failed to clone : " << error;
        BOOST_THROW_EXCEPTION(CloneFailedException(ss.str()));
    }
    this->childPtr = std::make_shared<boost::process::child>(pid);*/
    int pid = fork();
    if (pid == 0) {
        try {
            SandboxFork::Child(this->inPipe, this->outPipe).executeHttpActionMessage(event);
            std::quick_exit(0);
        } catch (...) {
            std::quick_exit(-1);
        }

        return keto::event::Event();
    }
    this->childPtr = std::make_shared<boost::process::child>(pid);
    return execute();
}

keto::event::Event SandboxFork::Parent::execute() {
    // use pipes in the reverse order from the child
    boost::process::ipstream pin(*this->outPipe);

    boost::process::opstream pout(*this->inPipe);

    while(childPtr->running()) {
        try {

            keto::wavm_common::ForkMessageWrapperHelper forkMessageWrapperHelper = read(pin);
            keto::wavm_common::ForkMessageWrapperHelper resultMessageWrapperHelper;
            resultMessageWrapperHelper.setCommand(forkMessageWrapperHelper.getCommand());
            if (forkMessageWrapperHelper.getCommand() == keto::wavm_common::ParentForkGateway::REQUEST::TRIGGER_EVENT) {
                try {
                    keto::event::Event request(forkMessageWrapperHelper.getEvent(), forkMessageWrapperHelper.getMessage());
                    keto::server_common::triggerEvent(request);
                } catch (keto::common::Exception &ex) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event : " << ex.what() << std::endl;
                    ss << "Cause: " << boost::diagnostic_information(ex, true) << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                } catch (boost::exception &ex) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event : "
                       << boost::diagnostic_information(ex, true) << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                } catch (std::exception &ex) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event : " << ex.what() << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                } catch (...) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event" << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                }
            } else if (forkMessageWrapperHelper.getCommand() ==
                       keto::wavm_common::ParentForkGateway::REQUEST::PROCESS_EVENT) {
                try {
                    keto::event::Event request(forkMessageWrapperHelper.getEvent(), forkMessageWrapperHelper.getMessage());
                    keto::event::Event result = keto::server_common::processEvent(request);
                    resultMessageWrapperHelper.setMessage(result.getMessage());
                } catch (keto::common::Exception &ex) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event : " << ex.what() << std::endl;
                    ss << "Cause: " << boost::diagnostic_information(ex, true) << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                } catch (boost::exception &ex) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event : "
                       << boost::diagnostic_information(ex, true) << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                } catch (std::exception &ex) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event : " << ex.what() << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                } catch (...) {
                    std::stringstream ss;
                    ss << "[SandboxFork::Parent::execute]Failed to process the event" << std::endl;
                    resultMessageWrapperHelper.setCommand(
                            keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION);
                    resultMessageWrapperHelper.setException(ss.str());
                }
            } else if (forkMessageWrapperHelper.getCommand() ==
                       keto::wavm_common::ParentForkGateway::REQUEST::RAISE_EXCEPTION) {
                // state exception handled and we are returning
                resultMessageWrapperHelper.setCommand(keto::wavm_common::ParentForkGateway::REQUEST::RETURN_RESULT);
                write(pout, resultMessageWrapperHelper);
                BOOST_THROW_EXCEPTION(ChildRequestException(forkMessageWrapperHelper.getException()));
            } else if (forkMessageWrapperHelper.getCommand() ==
                       keto::wavm_common::ParentForkGateway::REQUEST::RETURN_RESULT) {
                write(pout, resultMessageWrapperHelper);
                return keto::event::Event(forkMessageWrapperHelper.getMessage());
            }
            write(pout, resultMessageWrapperHelper);
        } catch (keto::common::Exception &ex) {
            std::stringstream ss;
            ss << "[SandboxFork::Parent::execute]Failed to process the event : " << ex.what() << std::endl;
            ss << "Cause: " << boost::diagnostic_information(ex, true) << std::endl;
            KETO_LOG_ERROR << ss.str();
            throw;
        } catch (boost::exception &ex) {
            std::stringstream ss;
            ss << "[SandboxFork::Parent::execute]Failed to process the event : "
               << boost::diagnostic_information(ex, true) << std::endl;
            KETO_LOG_ERROR << ss.str();
            throw;
        } catch (std::exception &ex) {
            std::stringstream ss;
            ss << "[SandboxFork::Parent::execute]Failed to process the event : " << ex.what() << std::endl;
            KETO_LOG_ERROR << ss.str();
            throw;
        } catch (...) {
            std::stringstream ss;
            ss << "[SandboxFork::Parent::execute]Failed to process the event" << std::endl;
            KETO_LOG_ERROR << ss.str();
            throw;
        }
    }
    std::stringstream ss;
    ss << "[SandboxFork::Parent::execute] The child process pid [" << this->childPtr->id()
        << "] exited unexpectedly : code [" << this->childPtr->exit_code()  << "]";
    KETO_LOG_ERROR << ss.str();
    BOOST_THROW_EXCEPTION(ChildExitedUnexpectedly(ss.str()));
}

void SandboxFork::Parent::write(boost::process::opstream& pout, const keto::wavm_common::ForkMessageWrapperHelper& forkMessageWrapperHelper) {
    std::vector<uint8_t> message = forkMessageWrapperHelper;
    //KETO_LOG_ERROR << "[SandboxFork::Parent::write] message size : " << message.size();
    pout << (size_t)message.size();
    pout.write((char*)message.data(),message.size());
    pout.flush();
    //KETO_LOG_ERROR << "[SandboxFork::Parent::write] hex is : " << Botan::hex_encode(message);
}

keto::wavm_common::ForkMessageWrapperHelper SandboxFork::Parent::read(boost::process::ipstream& pin) {
    size_t size;
    pin >> size;
    //KETO_LOG_ERROR << "[SandboxFork::Parent::read] Read the bytes in until EOF : " << size;
    std::vector<uint8_t> message(size);
    pin.read((char*)message.data(),size);
    //KETO_LOG_ERROR << "[SandboxFork::Parent::read] read the message from the pipe : " << message.size();
    //KETO_LOG_ERROR << "[SandboxFork::Parent::read] read in the buffer size : " << message.size();
    //KETO_LOG_ERROR << "[SandboxFork::Parent::read] hex is : " << Botan::hex_encode(message);
    return keto::wavm_common::ForkMessageWrapperHelper(message);
}

SandboxFork::SandboxFork(const keto::event::Event& event) : event(event){

}

SandboxFork::~SandboxFork() {

}

keto::event::Event SandboxFork::executeActionMessage() {
    return Parent().executeActionMessage(this->event);
}

keto::event::Event SandboxFork::executeHttpActionMessage() {
    return Parent().executeHttpActionMessage(this->event);
}




}
}
