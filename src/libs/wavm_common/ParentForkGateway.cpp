//
// Created by Brett Chaldecott on 2019/12/17.
//


#include <botan/hex.h>

#include <keto/server_common/VectorUtils.hpp>
#include "keto/wavm_common/ParentForkGateway.hpp"
#include "keto/wavm_common/Exception.hpp"


namespace keto {
namespace wavm_common {

static ParentForkGatewayPtr singleton;

std::string ParentForkGateway::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

const char* ParentForkGateway::REQUEST::RAISE_EXCEPTION = "EXCEPTION";
const char* ParentForkGateway::REQUEST::PROCESS_EVENT   = "PROCESS";
const char* ParentForkGateway::REQUEST::TRIGGER_EVENT   = "TRIGGER";
const char* ParentForkGateway::REQUEST::RETURN_RESULT   = "RETURN";

ParentForkGateway::ParentForkGateway(const PipePtr& inPipe, const PipePtr& outPipe) : pin(*inPipe), pout(*outPipe) {

}

ParentForkGateway::~ParentForkGateway() {
}

ParentForkGatewayPtr ParentForkGateway::init(const PipePtr& inPipe, const PipePtr& outPipe) {
    return singleton = ParentForkGatewayPtr(new ParentForkGateway(inPipe,outPipe));
}

void ParentForkGateway::fin() {
    singleton.reset();
}

void ParentForkGateway::raiseException(const std::string& exception) {
    ParentForkGateway::getInstance()->_raiseException(exception);
}

keto::event::Event ParentForkGateway::processEvent(const keto::event::Event& event) {
    return ParentForkGateway::getInstance()->_processEvent(event);
}

void ParentForkGateway::triggerEvent(const keto::event::Event& event) {
    ParentForkGateway::getInstance()->_triggerEvent(event);
}

void ParentForkGateway::returnResult(const keto::event::Event& event) {
    ParentForkGateway::getInstance()->_returnResult(event);
}

ParentForkGatewayPtr ParentForkGateway::getInstance() {
    return singleton;
}

void ParentForkGateway::_raiseException(const std::string& exception) {
    ForkMessageWrapperHelper forkMessageWrapperHelper;
    forkMessageWrapperHelper.setCommand(ParentForkGateway::REQUEST::RAISE_EXCEPTION);
    forkMessageWrapperHelper.setMessage(exception);

    write(forkMessageWrapperHelper);
    ForkMessageWrapperHelper resultMessage = read();

    if (resultMessage.getCommand() == ParentForkGateway::REQUEST::RAISE_EXCEPTION) {
        BOOST_THROW_EXCEPTION(ParentRequestException(resultMessage.getException()));
    }
}

keto::event::Event ParentForkGateway::_processEvent(const keto::event::Event& event) {
    ForkMessageWrapperHelper forkMessageWrapperHelper;
    forkMessageWrapperHelper.setCommand(ParentForkGateway::REQUEST::PROCESS_EVENT);
    forkMessageWrapperHelper.setEvent(event.getName());
    forkMessageWrapperHelper.setMessage(event.getMessage());

    write(forkMessageWrapperHelper);
    ForkMessageWrapperHelper resultMessage = read();

    if (resultMessage.getCommand() == ParentForkGateway::REQUEST::RAISE_EXCEPTION) {
        BOOST_THROW_EXCEPTION(ParentRequestException(resultMessage.getException()));
    }
    return keto::event::Event(resultMessage.getMessage());
}

void ParentForkGateway::_triggerEvent(const keto::event::Event& event) {
    ForkMessageWrapperHelper forkMessageWrapperHelper;
    forkMessageWrapperHelper.setCommand(ParentForkGateway::REQUEST::TRIGGER_EVENT);
    forkMessageWrapperHelper.setEvent(event.getName());
    forkMessageWrapperHelper.setMessage(event.getMessage());

    write(forkMessageWrapperHelper);
    ForkMessageWrapperHelper resultMessage = read();

    if (resultMessage.getCommand() == ParentForkGateway::REQUEST::RAISE_EXCEPTION) {
        BOOST_THROW_EXCEPTION(ParentRequestException(resultMessage.getException()));
    }
}

void ParentForkGateway::_returnResult(const keto::event::Event& event) {
    ForkMessageWrapperHelper forkMessageWrapperHelper;
    forkMessageWrapperHelper.setCommand(ParentForkGateway::REQUEST::RETURN_RESULT);
    forkMessageWrapperHelper.setMessage(event.getMessage());

    write(forkMessageWrapperHelper);
    ForkMessageWrapperHelper resultMessage = read();
    // no need to parse the return result we just process it to make sure the parent has parsed it successfully
}

keto::wavm_common::ForkMessageWrapperHelper ParentForkGateway::read() {
    //KETO_LOG_ERROR << "[ParentForkGateway::read] Read the bytes in until EOF";
    while(true) {
        size_t size;
        pin >> size;
        if (!size) {
            continue;
        }
        //KETO_LOG_ERROR << "[ParentForkGateway::read] Read the bytes in until EOF : " << size;
        std::vector<uint8_t> message(size);
        pin.read((char *) message.data(), size);
        //KETO_LOG_ERROR << "[ParentForkGateway::read] read in the buffer size : " << message.size();
        return keto::wavm_common::ForkMessageWrapperHelper(message);
    }
}

void ParentForkGateway::write(const keto::wavm_common::ForkMessageWrapperHelper& forkMessageWrapperHelper) {
    std::vector<uint8_t> message = forkMessageWrapperHelper;
    //KETO_LOG_ERROR << "[ParentForkGateway::write] message size : " << message.size();
    pout << (size_t)message.size();
    pout.write((char*)message.data(),message.size());
    pout.flush();
    //KETO_LOG_ERROR << "[ParentForkGateway::write] hex is : " << Botan::hex_encode(message);
}

}
}