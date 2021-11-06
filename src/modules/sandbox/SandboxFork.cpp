//
// Created by Brett Chaldecott on 2019/12/17.
//

#include "keto/sandbox/SandboxFork.hpp"
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <thread>

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


const std::string SandboxFork::PING("ping");
const std::string SandboxFork::PONG("pong");
const std::string SandboxFork::TERMINATE("terminate");
const std::string SandboxFork::EXECUTE_ACTION("execute_action");
const std::string SandboxFork::EXECUTE_ACTION_MESSAGE("execute_action_message");
const std::string SandboxFork::EXECUTE_HTTP("execute_http");
const std::string SandboxFork::EXECUTE_HTTP_MESSAGE("execute_http_message");

std::string SandboxFork::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

// a mutex to lock the creation of forks
static std::mutex forkMutex;

SandboxFork::Child::Child(const keto::wavm_common::PipePtr& inPipe, const keto::wavm_common::PipePtr& outPipe) {
    keto::wavm_common::ParentForkGateway::init(inPipe,outPipe);
    keto::wavm_common::ParentForkGateway::ping();
}

SandboxFork::Child::~Child() {
    // exit this method
    keto::wavm_common::ParentForkGateway::fin();
}

void SandboxFork::Child::execute() {
    while(true) {
        std::shared_ptr<std::string> command = keto::wavm_common::ParentForkGateway::getCommand();
        if (!command) {
            KETO_LOG_ERROR << "[SandboxFork::Child::execute] Invalid command provided";
        } else if (*command == SandboxFork::TERMINATE) {
            break;
        } else if (*command == SandboxFork::EXECUTE_ACTION) {
            executeActionMessage(keto::wavm_common::ParentForkGateway::getRequest());
        } else if (*command == SandboxFork::EXECUTE_HTTP) {
            keto::sandbox::SandboxFork::Child::executeHttpActionMessage(keto::wavm_common::ParentForkGateway::getRequest());
        }
    }
}

keto::event::Event SandboxFork::Child::executeActionMessage(const keto::event::Event& event) {
    try {
        //KETO_LOG_INFO << "[executeActionMessage] setup engine";
        keto::proto::SandboxCommandMessage sandboxCommandMessage =
                keto::server_common::fromEvent<keto::proto::SandboxCommandMessage>(event);
        keto::wavm_common::WavmSessionScope wavmSessionScope(sandboxCommandMessage);
        std::string buffer = sandboxCommandMessage.contract();
        std::string code = keto::server_common::VectorUtils().copyVectorToString(Botan::hex_decode(
                buffer,true));
        //KETO_LOG_INFO << "[executeActionMessage] Execute the engine";
        keto::wavm_common::WavmEngineManager::getInstance()->getEngine(code,sandboxCommandMessage.contract_name())->execute();
        //KETO_LOG_INFO << "[executeActionMessage] Get the sandbox message";
        sandboxCommandMessage = std::dynamic_pointer_cast<keto::wavm_common::WavmSessionTransaction>(wavmSessionScope.getSession())->getSandboxCommandMessage();
        //KETO_LOG_INFO << "[executeActionMessage] return result";
        keto::wavm_common::ParentForkGateway::returnResult(
                keto::server_common::toEvent<keto::proto::SandboxCommandMessage>(sandboxCommandMessage));
        //KETO_LOG_INFO << "[executeActionMessage] after execution";
    } catch (keto::common::Exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        ss << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    } catch (boost::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeActionMessage]Failed to process the contract : " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    } catch (std::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    } catch (...) {
        std::stringstream ss;
        ss << "[[SandboxFork::Child::executeActionMessage]]Failed to process the contract" << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    }
    return keto::event::Event();
}

keto::event::Event SandboxFork::Child::executeHttpActionMessage(const keto::event::Event& event) {
    try {
        //KETO_LOG_INFO << "[executeActionMessage] Retrieve the message";
        keto::proto::HttpRequestMessage httpRequestMessage =
                keto::server_common::fromEvent<keto::proto::HttpRequestMessage>(event);
        keto::wavm_common::WavmSessionScope wavmSessionScope(httpRequestMessage);
        std::string buffer = httpRequestMessage.contract();
        std::string code = keto::server_common::VectorUtils().copyVectorToString(Botan::hex_decode(
                buffer,true));
        //KETO_LOG_INFO << "[executeActionMessage] Execute the engine";
        keto::wavm_common::WavmEngineManager::getInstance()->getEngine(code,httpRequestMessage.contract_name())->executeHttp();
        //KETO_LOG_INFO << "[executeActionMessage] Get the resource";
        keto::wavm_common::ParentForkGateway::returnResult(keto::server_common::toEvent<keto::proto::HttpResponseMessage>(
                std::dynamic_pointer_cast<keto::wavm_common::WavmSessionHttp>(wavmSessionScope.getSession())->getHttpResponse()));

    } catch (keto::common::Exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        ss << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR <<   ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    } catch (boost::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    } catch (std::exception& ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract : " << ex.what() << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    } catch (...) {
        std::stringstream ss;
        ss << "[SandboxFork::Child::executeHttpActionMessage]Failed to process the contract" << std::endl;
        KETO_LOG_ERROR << ss.str();
        keto::wavm_common::ParentForkGateway::raiseException(ss.str(),false);
    }
    return keto::event::Event();
}

SandboxFork::ParentStream::ParentStream(keto::wavm_common::PipePtr inPipe, keto::wavm_common::PipePtr outPipe) :
    pin(*inPipe), pout(*outPipe) {

}

SandboxFork::ParentStream::~ParentStream() {

}

boost::process::ipstream& SandboxFork::ParentStream::getPin() {
    return this->pin;
}

boost::process::opstream& SandboxFork::ParentStream::getPout() {
    return this->pout;
}

SandboxFork::Parent::Parent()
    : inPipe(new boost::process::pipe()), outPipe(new boost::process::pipe()) {
    for (int count = 0; count < 5; count++) {
        //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage] Before fork";
        pid_t pid = fork();
        //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage] after fork : " << pid;
        if (pid < 0) {
            KETO_LOG_ERROR << "[SandboxFork::Parent::executeActionMessage] fork creation failed :" << pid;
            BOOST_THROW_EXCEPTION(ForkException());
        } else if (pid == 0) {
            try {
                //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage] Execute the child process";
                SandboxFork::Child(this->inPipe, this->outPipe).execute();
                //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage] Child processing complete";
            } catch (...) {
                KETO_LOG_ERROR << "[SandboxFork::Parent::executeActionMessage] failed execute correctly unexpected exception. Child is terminating";
            }
            std::quick_exit(0);
        }
        //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage]Validate the fork";
        this->childPtr = std::make_shared<boost::process::child>(boost::process::detail::posix::child_handle(pid));
        this->parentStream = std::make_shared<SandboxFork::ParentStream>(this->outPipe,this->inPipe);
        if (validateFork(this->parentStream->getPin(),this->parentStream->getPout())) {
            KETO_LOG_ERROR << "[SandboxFork::Parent::executeActionMessage] The fork is running correctly";
            break;
        }
        //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage] The fork is invalid retry";
        this->childPtr->terminate();
        this->inPipe = keto::wavm_common::PipePtr(new boost::process::pipe());
        this->outPipe = keto::wavm_common::PipePtr(new boost::process::pipe());
        continue;
    }
}

SandboxFork::Parent::~Parent() {
    try {
        if (childPtr) {
            std::error_code ec;
            if (!childPtr->wait_for(std::chrono::seconds(10),ec)) {
                KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Child failed to exit cleanly : " << ec.message();
                ec.clear();
                childPtr->terminate(ec);
                if (ec) {
                    KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Failed to terminate the child : " << ec.message();
                }
            }
            childPtr.reset();
        }
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Failed to process the contract : " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Failed to process the contract : " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Failed to process the contract : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[SandboxFork::Parent::~Parent]Failed to process the contract";
    }
}

keto::event::Event SandboxFork::Parent::executeActionMessage(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage]Execute in the fork";
    write(parentStream->getPout(),SandboxFork::EXECUTE_ACTION);
    std::shared_ptr<std::string> actionResponse = readText(parentStream->getPin());
    if (!actionResponse && *actionResponse != keto::wavm_common::ParentForkGateway::REQUEST::EXECUTE_CONFIRM) {
        BOOST_THROW_EXCEPTION(ActionExecutionException());
    }
    keto::wavm_common::ForkMessageWrapperHelper forkMessageWrapperHelper;
    //forkMessageWrapperHelper.setCommand(SandboxFork::EXECUTE_ACTION_MESSAGE);
    forkMessageWrapperHelper.setMessage(event.getMessage());
    write(parentStream->getPout(),forkMessageWrapperHelper);
    std::shared_ptr<std::string> messageResponse = readText(parentStream->getPin());
    if (!messageResponse || *messageResponse != keto::wavm_common::ParentForkGateway::REQUEST::MESSAGE_CONFIRM) {
        BOOST_THROW_EXCEPTION(ActionExecutionException());
    }
    return execute(parentStream->getPin(),parentStream->getPout());
}

keto::event::Event SandboxFork::Parent::executeHttpActionMessage(const keto::event::Event& event) {
    //KETO_LOG_INFO << "[SandboxFork::Parent::executeActionMessage]Execute in the fork";
    write(parentStream->getPout(),SandboxFork::EXECUTE_HTTP);
    std::shared_ptr<std::string> actionResponse = readText(parentStream->getPin());
    if (!actionResponse || *actionResponse != keto::wavm_common::ParentForkGateway::REQUEST::EXECUTE_CONFIRM) {
        BOOST_THROW_EXCEPTION(HttpExecutionException());
    }
    keto::wavm_common::ForkMessageWrapperHelper forkMessageWrapperHelper;
    forkMessageWrapperHelper.setCommand(SandboxFork::EXECUTE_HTTP_MESSAGE);
    forkMessageWrapperHelper.setMessage(event.getMessage());
    write(parentStream->getPout(),forkMessageWrapperHelper);
    std::shared_ptr<std::string> messageResponse = readText(parentStream->getPin());
    if (!messageResponse || *messageResponse != keto::wavm_common::ParentForkGateway::REQUEST::MESSAGE_CONFIRM) {
        BOOST_THROW_EXCEPTION(ActionExecutionException());
    }
    return execute(parentStream->getPin(),parentStream->getPout());
}

bool SandboxFork::Parent::terminate() {
    //KETO_LOG_INFO << "[SandboxFork::Parent::terminate] terminate the parent";
    write(parentStream->getPout(),SandboxFork::TERMINATE);
    //KETO_LOG_INFO << "[SandboxFork::Parent::terminate] read some data back from the child";
    std::shared_ptr<std::string> terminateResponse = readText(parentStream->getPin());
    if (terminateResponse && *terminateResponse == keto::wavm_common::ParentForkGateway::REQUEST::EXECUTE_CONFIRM) {
        return true;
    }
    return false;
}

bool SandboxFork::Parent::validateFork(boost::process::ipstream& pin, boost::process::opstream& pout) {
    //KETO_LOG_INFO << "[SandboxFork::Parent::validateFork] ping the child";
    write(pout,"ping");
    //KETO_LOG_INFO << "[SandboxFork::Parent::validateFork] read some data back from the child";
    std::shared_ptr<std::string> pingResponse = readText(pin);
    if (!pingResponse) {
        return false;
    }
    //KETO_LOG_INFO << "[SandboxFork::Parent::validateFork] response is : " << *pingResponse;
    return true;
}

keto::event::Event SandboxFork::Parent::execute(boost::process::ipstream& pin, boost::process::opstream& pout) {
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
            //KETO_LOG_ERROR << "Ignore these exceptions as they indicate an invalid contract";
            throw;
            //break;
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
    pout << (size_t)message.size();
    pout.write((char*)message.data(),message.size());
    pout.flush();
}

void SandboxFork::Parent::write(boost::process::opstream& pout, const std::string message) {
    pout << (size_t)message.size();
    pout.write((char*)message.data(),message.size());
    pout.flush();
}

keto::wavm_common::ForkMessageWrapperHelper SandboxFork::Parent::read(boost::process::ipstream& pin) {
    size_t size;
    pin >> size;
    //KETO_LOG_DEBUG << "The size is [" << size << "]";
    std::vector<uint8_t> message(size);
    pin.read((char *) message.data(), size);
    //KETO_LOG_DEBUG << "The size is [" << size << "][" << pin.gcount() << "]";
    try {
        return keto::wavm_common::ForkMessageWrapperHelper(message);
    } catch (boost::exception &ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Parent::read] failed deserialize the object : "
           << boost::diagnostic_information(ex, true) << std::endl;
        KETO_LOG_ERROR << ss.str();
        throw;
    } catch (std::exception &ex) {
        std::stringstream ss;
        ss << "[SandboxFork::Parent::read] failed deserialize the object : " << ex.what() << std::endl;
        KETO_LOG_ERROR << ss.str();
        throw;
    } catch (...) {
        std::stringstream ss;
        ss << "[SandboxFork::Parent::read] failed deserialize the object : " << std::endl;
        KETO_LOG_ERROR << ss.str();
        throw;
    }
}

std::shared_ptr<std::string> SandboxFork::Parent::readText(boost::process::ipstream& pin) {
    //if (pin.sync() == -1) {
    //    return std::shared_ptr<std::string>();
    //}
    //KETO_LOG_ERROR << "[SandboxFork::Parent::readSome] Check for data";
    //for (int count = 0; count < 20; count++) {
        //KETO_LOG_INFO << "[SandboxFork::Parent::readSome]Check for data";
        //if (pin.rdbuf()->in_avail()) {
            //KETO_LOG_INFO << "[SandboxFork::Parent::readSome] Read data";
            size_t size;
            pin >> size;
            std::vector<uint8_t> message(size);
            pin.read((char *) message.data(), size);
            //KETO_LOG_INFO << "[SandboxFork::Parent::readSome] Return the data";
            return std::make_shared<std::string>((char*)message.data(),message.size());
        //}
        //KETO_LOG_INFO << "[SandboxFork::Parent::readSome] No data available : " << pin.rdbuf()->in_avail();
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //}
    //KETO_LOG_ERROR << "[SandboxFork::Parent::readSome] Failed to read in entries from the pipe";
    //return std::shared_ptr<std::string>();
    //if (pin.read((char *) message.data(), size) == -1) {
    //    return std::shared_ptr<std::string>();
    //}

}

SandboxFork::SandboxFork() : usageCount(0) {

}

SandboxFork::~SandboxFork() {

}

int SandboxFork::getUsageCount() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return usageCount;
}

int SandboxFork::incrementUsageCount() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return usageCount++;
}

keto::event::Event SandboxFork::executeActionMessage(const keto::event::Event& event) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->parent.executeActionMessage(event);
}

keto::event::Event SandboxFork::executeHttpActionMessage(const keto::event::Event& event) {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->parent.executeHttpActionMessage(event);
}

bool SandboxFork::terminate() {
    std::unique_lock<std::mutex> uniqueLock(this->classMutex);
    return this->parent.terminate();
}



}
}
