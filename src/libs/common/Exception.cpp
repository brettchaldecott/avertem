/* 
 * File:   EnvironmentManager.cpp
 * Author: ubuntu
 * 
 * Created on January 10, 2018, 12:15 PM
 */


#include "keto/common/Exception.hpp"

namespace keto {
namespace common {  

Exception::Exception() noexcept : type("Exception"), msg("Exception"){}

Exception::Exception(const std::string& msg) noexcept: type("Exception"), msg(msg) {}

Exception::Exception(const std::string& type, const std::string& msg) noexcept : type(type), msg(msg) {}

Exception::Exception(const Exception& orig) noexcept : std::exception(orig), boost::exception(orig), type(orig.type), msg(orig.msg) {}

Exception::~Exception() {
}

Exception& Exception::operator= (const Exception& orig) noexcept {
    this->type = orig.type;
    this->msg = orig.msg;

	return *this;
}

std::string Exception::getType() const noexcept {
    return this->type;
}
	
const char* Exception::what() const noexcept {
	return this->msg.c_str();
}


}
}
