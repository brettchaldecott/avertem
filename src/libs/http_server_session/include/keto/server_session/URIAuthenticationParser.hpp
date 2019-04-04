//
// Created by Brett Chaldecott on 2019/03/26.
//

#ifndef KETO_URIAUTHENTICATIONPARSER_HPP
#define KETO_URIAUTHENTICATIONPARSER_HPP

#include <string>
#include <map>

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"


namespace keto {
namespace server_session {

class URIAuthenticationParser {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();


    URIAuthenticationParser(const std::string& uri);
    URIAuthenticationParser(const URIAuthenticationParser& orig) = default;
    virtual ~URIAuthenticationParser();

    bool isCors();
    keto::asn1::HashHelper getAccountHash();
    keto::asn1::HashHelper getSourceHash();
    std::vector<uint8_t> getSignature();


private:
    bool cors;
    keto::asn1::HashHelper accountHash;
    keto::asn1::HashHelper sourceHash;
    std::string signature;

};


}
}


#endif //KETO_URIAUTHENTICATIONPARSER_HPP
