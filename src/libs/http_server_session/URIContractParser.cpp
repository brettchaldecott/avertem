//
// Created by Brett Chaldecott on 2019/03/06.
//

#include <sstream>
#include <iostream>

#include <boost/n

#include "keto/common/HttpEndPoints.hpp"
#include "keto/server_session/URIContractParser.hpp"


namespace keto {
namespace server_session {


std::string URIContractParser::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

URIContractParser::URIContractParser(const std::string &uri) {
    std::string subUri = uri.substr(strlen(keto::common::HttpEndPoints::CONTRACT));
    int nextSlash = subUri.find("/");
    this->contractHash = subUri.substr(0,nextSlash);
    subUri = subUri.substr(nextSlash + 1);
    int queryStart = subUri.find(QUERY_START);
    if (queryStart == std::string::npos) {
        this->requestUri = subUri;
    } else {
        this->requestUri = subUri.substr(0,queryStart);
        this->query = subUri.substr(queryStart+1);
    }

    std::istringstream iss(this->query);
    std::string token;
    while (std::getline(iss, token, QUERY_SEPERATOR)) {
        int pos = token.find(QUERY_ENTRY_SEPERATOR);
        if (pos == std::string::npos) {
            this->parameters[token] = "";
        } else {
            std::string key = token.substr(0,pos);
            this->parameters[key] = urlDecode(token.substr(pos+1));
        }
    }

}

URIContractParser::~URIContractParser() {

}

std::string URIContractParser::getContractHash() {
    return this->contractHash;
}

std::string URIContractParser::getQuery() {
    return this->query;
}

std::string URIContractParser::getRequestUri() {
    return this->requestUri;
}

ParameterMap URIContractParser::getParameters() {
    return this->parameters;
}

std::string URIContractParser::urlDecode(std::string text) {
    char h;
    std::ostringstream escaped;
    escaped.fill('0');

    for (auto i = text.begin(), n = text.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        if (c == '%') {
            if (i[1] && i[2]) {
                h = from_hex(i[1]) << 4 | from_hex(i[2]);
                escaped << h;
                i += 2;
            }
        } else if (c == '+') {
            escaped << ' ';
        } else {
            escaped << c;
        }
    }

    return escaped.str();
}

char URIContractParser::from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}



}
}