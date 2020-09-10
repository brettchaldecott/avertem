//
// Created by Brett Chaldecott on 2019/12/18.
//


#include "keto/server_common/VectorUtils.hpp"
#include "keto/wavm_common/ForkMessageWrapperHelper.hpp"
#include "keto/wavm_common/Exception.hpp"

namespace keto {
namespace wavm_common {

std::string ForkMessageWrapperHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ForkMessageWrapperHelper::ForkMessageWrapperHelper() {

}

ForkMessageWrapperHelper::ForkMessageWrapperHelper(const std::string& message) {
    if (!this->forkMessageWrapper.ParseFromString(message)) {
        BOOST_THROW_EXCEPTION(FailedToParseFormMessage());
    }
}

ForkMessageWrapperHelper::ForkMessageWrapperHelper(std::istream* stream) {
    if (!this->forkMessageWrapper.ParseFromIstream(stream)) {
        BOOST_THROW_EXCEPTION(FailedToParseFormMessage());
    }
}

ForkMessageWrapperHelper::ForkMessageWrapperHelper(const std::vector<uint8_t>& message) {
    if (!this->forkMessageWrapper.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(message))) {
        BOOST_THROW_EXCEPTION(FailedToParseFormMessage());
    }
}

ForkMessageWrapperHelper::ForkMessageWrapperHelper(const keto::proto::ForkMessageWrapper& forkMessageWrapper) :
    forkMessageWrapper(forkMessageWrapper) {

}

ForkMessageWrapperHelper::~ForkMessageWrapperHelper() {

}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::setCommand(const std::string& command) {
    forkMessageWrapper.set_command(command);
    return *this;
}

std::string ForkMessageWrapperHelper::getCommand() {
    return forkMessageWrapper.command();
}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::setEvent(const std::string& event) {
    forkMessageWrapper.set_event(event);
    return *this;
}

std::string ForkMessageWrapperHelper::getEvent() {
    return forkMessageWrapper.event();
}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::setMessage(const std::string& message) {
    forkMessageWrapper.set_message(message);
    return *this;
}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::setMessage(const std::vector<uint8_t>& event) {
    forkMessageWrapper.set_message(keto::server_common::VectorUtils().copyVectorToString(event));
    return *this;
}

std::string ForkMessageWrapperHelper::getMessage() {
    return forkMessageWrapper.message();
}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::setException(const std::string& exception) {
    forkMessageWrapper.set_exception(exception);
    return *this;
}

std::string ForkMessageWrapperHelper::getException() {
    return forkMessageWrapper.exception();
}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::operator = (const std::string& message) {
    if (!this->forkMessageWrapper.ParseFromString(message)) {
        BOOST_THROW_EXCEPTION(FailedToParseFormMessage());
    }
    return *this;
}

ForkMessageWrapperHelper& ForkMessageWrapperHelper::operator = (const keto::proto::ForkMessageWrapper& forkMessageWrapper) {
    this->forkMessageWrapper = forkMessageWrapper;
    return *this;
}

ForkMessageWrapperHelper::operator keto::proto::ForkMessageWrapper() const {
    return this->forkMessageWrapper;
}

ForkMessageWrapperHelper::operator std::string() const {
    return forkMessageWrapper.SerializeAsString();
}

ForkMessageWrapperHelper::operator std::vector<uint8_t>() const {
    return keto::server_common::VectorUtils().copyStringToVector(forkMessageWrapper.SerializeAsString());
}

}
}