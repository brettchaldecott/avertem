//
// Created by Brett Chaldecott on 2019/03/06.
//

#ifndef KETO_URICONTRACTPARSER_HPP
#define KETO_URICONTRACTPARSER_HPP

#include <string>
#include <map>

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace server_session {

typedef std::map<std::string,std::string> ParameterMap;

class URIContractParser {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    static constexpr const char* QUERY_START = "?";
    static constexpr const char QUERY_SEPERATOR = '&';
    static constexpr const char* QUERY_ENTRY_SEPERATOR = "=";

    URIContractParser(const std::string& uri);
    URIContractParser(const URIContractParser& orig) = default;
    virtual ~URIContractParser();

    bool isCors();
    bool hasSessionHash();
    std::string getSessionHash();
    std::string getContractHash();
    std::string getQuery();
    std::string getRequestUri();
    ParameterMap getParameters();


private:
    bool cors;
    std::string sessionHash;
    std::string contractHash;
    std::string query;
    std::string requestUri;
    ParameterMap parameters;


    std::string urlDecode(std::string str);
    char from_hex(char ch);

};


}
}


#endif //KETO_URICONTRACTPARSER_HPP
