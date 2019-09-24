//
// Created by Brett Chaldecott on 2019-09-21.
//

#ifndef KETO_URIBLOCKCHAINEXPLORERPARSER_HPP
#define KETO_URIBLOCKCHAINEXPLORERPARSER_HPP

#include <string>
#include <map>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"


namespace keto {
namespace server_session {


class URIBlockchainExplorerParser {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    static constexpr const char* BLOCK_QUERY = "block/";

    static constexpr const char* URL_SEP = "/";
    static constexpr const char* TRANSACTION_QUERY = "transaction/";
    static constexpr const char* TRANSACTION_QUERY_BLOCK    = "block/";
    static constexpr const char* TRANSACTION_QUERY_TRANS    = "trans/";
    static constexpr const char* TRANSACTION_QUERY_ACCOUNT  = "account/";
    static constexpr const char* LATEST_HASH = "latest/";

    static constexpr const char* PRODUCER_QUERY = "producer/";

    // query parameter pos
    static constexpr const int QUERY_HASH_POS = 0;
    static constexpr const int QUERY_NUMBER = 1;


    URIBlockchainExplorerParser(const std::string& uri);
    URIBlockchainExplorerParser(const URIBlockchainExplorerParser& orig) = default;
    virtual ~URIBlockchainExplorerParser();

    std::string getUri() const;

    bool isCors() const;
    keto::asn1::HashHelper getSessionHash() const;
    bool isBlockQuery() const;
    bool isTransactionQuery() const;
    bool isProducerQuery() const;

    // transaction query parameters
    bool isBlockTransactionQuery() const;
    bool isTransTransactionQuery() const;
    bool isAccountTransactionQuery() const;

    keto::asn1::HashHelper getHash() const;
    int getNumberOfEntries() const;

private:
    std::string uri;
    bool cors;
    keto::asn1::HashHelper sessionHash;
    bool blockQuery;
    bool transactionQuery;
    bool producerQuery;

    // transaction hashes
    bool blockTransactionQuery;
    bool transTransactionQuery;
    bool accountTransactionQuery;


    // query parameters
    bool latest;
    keto::asn1::HashHelper hash;
    int numberOfEntries;



};


}
}

#endif //KETO_URIBLOCKCHAINEXPLORERPARSER_HPP
