//
// Created by Brett Chaldecott on 2019/03/26.
//

#include <sstream>
#include <iostream>

#include <botan/hex.h>

#include "keto/common/HttpEndPoints.hpp"
#include "keto/server_session/URIAuthenticationParser.hpp"
#include "keto/server_common/StringUtils.hpp"

#include "keto/server_session/Exception.hpp"


namespace keto {
namespace server_session {

std::string URIAuthenticationParser::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


URIAuthenticationParser::URIAuthenticationParser(const std::string& uri) {
    std::string subUri = uri.substr(strlen(keto::common::HttpEndPoints::AUTHENTICATE));
    std::vector<std::string> values = keto::server_common::StringUtils(subUri).tokenize("/");
    if (values.size() != 3) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidAuthenticationURL());
    }
    int nextSlash = subUri.find("/");
    this->accountHash = keto::asn1::HashHelper(values[0],keto::common::HEX);
    this->sourceHash = keto::asn1::HashHelper(values[1],keto::common::HEX);
    this->signature = values[2];
}

URIAuthenticationParser::~URIAuthenticationParser() {

}

keto::asn1::HashHelper URIAuthenticationParser::getAccountHash() {
    return this->accountHash;
}

keto::asn1::HashHelper URIAuthenticationParser::getSourceHash() {
    return this->sourceHash;
}

std::vector<uint8_t> URIAuthenticationParser::getSignature() {
    return Botan::hex_decode(this->signature,true);
}

}
}