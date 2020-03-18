//
// Created by Brett Chaldecott on 2019/03/06.
//

#ifndef KETO_HTTPCONTRACTMANAGER_HPP
#define KETO_HTTPCONTRACTMANAGER_HPP

#include <string>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "Sandbox.pb.h"
#include "Contract.pb.h"
#include "BlockChain.pb.h"

#include "keto/asn1/HashHelper.hpp"

#include "keto/server_session/HttpSessionManager.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace server_session {


class HttpContractManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    HttpContractManager(std::shared_ptr<HttpSessionManager>& httpSessionManagerPtr);
    HttpContractManager(const HttpContractManager& orig) = delete;
    virtual ~HttpContractManager();

    boost::beast::http::response<boost::beast::http::string_body> processQuery(
            boost::beast::http::request<boost::beast::http::string_body>& req,
            const std::string& body);

private:
    std::shared_ptr<HttpSessionManager> httpSessionManagerPtr;

    keto::proto::ContractMessage getContractByHash(const keto::asn1::HashHelper& account, const std::string& hash);
    keto::proto::ContractMessage getContract(keto::proto::ContractMessage& contractMessage);
    void copyHttpParameters(
            std::map<std::string,std::string> parameters,
            keto::proto::HttpRequestMessage& httpRequestMessage);

};

}
}


#endif //KETO_HTTPCONTRACTMANAGER_HPP
