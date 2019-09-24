//
// Created by Brett Chaldecott on 2019-09-19.
//

#ifndef KETO_HTTPBLOCKCHAINEXPLORERMANAGER_HPP
#define KETO_HTTPBLOCKCHAINEXPLORERMANAGER_HPP

#include <string>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "keto/server_session/HttpSessionManager.hpp"
#include "keto/server_session/URIBlockchainExplorerParser.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace server_session {

class HttpBlockchainExplorerManager;
typedef std::shared_ptr<HttpBlockchainExplorerManager> HttpBlockchainExplorerManagerPtr;

class HttpBlockchainExplorerManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    HttpBlockchainExplorerManager(std::shared_ptr<HttpSessionManager>& httpSessionManagerPtr);
    HttpBlockchainExplorerManager(const HttpBlockchainExplorerManager& orig) = delete;
    virtual ~HttpBlockchainExplorerManager();

    std::string processQuery(
            boost::beast::http::request<boost::beast::http::string_body>& req,
            const std::string& body);
public:
    std::shared_ptr<HttpSessionManager> httpSessionManagerPtr;

    std::string processBlockQuery(const URIBlockchainExplorerParser& uriBlockchainExplorerParser);
    std::string processTransactionQuery(const URIBlockchainExplorerParser& uriBlockchainExplorerParser);
    std::string processProducerQuery(const URIBlockchainExplorerParser& uriBlockchainExplorerParser);
};

}
}



#endif //KETO_HTTPBLOCKCHAINEXPLORERMANAGER_HPP
