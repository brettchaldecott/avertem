//
// Created by Brett Chaldecott on 2019-09-21.
//

#include "keto/server_session/URIBlockchainExplorerParser.hpp"
#include "keto/server_session/Exception.hpp"

#include "keto/server_common/StringUtils.hpp"

#include "keto/common/HttpEndPoints.hpp"

namespace keto {
namespace server_session {

std::string URIBlockchainExplorerParser::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

URIBlockchainExplorerParser::URIBlockchainExplorerParser(const std::string& uri) : cors(false), blockQuery(false),
        transactionQuery(false), blockTransactionQuery(false), transTransactionQuery(false), accountTransactionQuery(false),
        latest(false),numberOfEntries(0) {
    // normalize the uri
    std::string subUri = this->uri = keto::server_common::StringUtils(uri).replaceAll("//","/").substr(strlen(keto::common::HttpEndPoints::BLOCK_EXPLORER_QUERY));

    // check if there is a cors prefix
    if (subUri.find(keto::common::HttpEndPoints::CORS_ENABLED) == 0) {
        this->cors = true;
        subUri = subUri.substr(strlen(keto::common::HttpEndPoints::CORS_ENABLED));
    }

    int end = 0;

    if (subUri.find(BLOCK_QUERY) == 0) {
        this->blockQuery = true;
        subUri = subUri.substr(strlen(BLOCK_QUERY));
    } else if (subUri.find(TRANSACTION_QUERY) == 0) {
        this->transactionQuery = true;
        subUri = subUri.substr(strlen(BLOCK_QUERY));
        if (subUri.find(TRANSACTION_QUERY_BLOCK) == 0) {
            this->blockTransactionQuery = true;
            subUri = subUri.substr(strlen(TRANSACTION_QUERY_BLOCK));
        } else if (subUri.find(TRANSACTION_QUERY_TRANS) == 0) {
            this->transTransactionQuery = true;
            subUri = subUri.substr(strlen(TRANSACTION_QUERY_TRANS));
        } else if (subUri.find(TRANSACTION_QUERY_ACCOUNT) == 0) {
            this->accountTransactionQuery = true;
            subUri = subUri.substr(strlen(TRANSACTION_QUERY_ACCOUNT));
        } else {
            BOOST_THROW_EXCEPTION(keto::server_session::InvalidBlockQueryByTransctionException());
        }
    } else if (subUri.find(PRODUCER_QUERY) == 0) {
        this->producerQuery = true;
        // clear the sub uri as we
        subUri.clear();
    } else {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidBlockQueryException());
    }

    // strip off the block
    if (subUri.find(LATEST_HASH) == 0) {
        this->latest = true;
        subUri = subUri.substr(strlen(LATEST_HASH));
    } else if (subUri.find(URL_SEP) != subUri.npos) {
        end = subUri.find(URL_SEP);
        this->hash = keto::asn1::HashHelper(subUri.substr(0,end),keto::common::StringEncoding::HEX);
    }

    end = subUri.size();
    if (subUri.find(URL_SEP) != subUri.npos) {
        end = subUri.find(URL_SEP);
    }
    std::string number = subUri.substr(0,end);
    if (!number.empty()) {
        this->numberOfEntries = std::stoi(number);
    }



}

URIBlockchainExplorerParser::~URIBlockchainExplorerParser() {

}

std::string URIBlockchainExplorerParser::getUri() const {
    return this->uri;
}

bool URIBlockchainExplorerParser::isCors() const {
    return this->cors;
}

bool URIBlockchainExplorerParser::isBlockQuery() const {
    return this->blockQuery;
}

bool URIBlockchainExplorerParser::isTransactionQuery() const {
    return this->transactionQuery;
}

bool URIBlockchainExplorerParser::isProducerQuery() const {
    return this->producerQuery;
}

// transaction query parameters
bool URIBlockchainExplorerParser::isBlockTransactionQuery() const {
    return this->blockTransactionQuery;
}

bool URIBlockchainExplorerParser::isTransTransactionQuery() const {
    return this->transTransactionQuery;
}

bool URIBlockchainExplorerParser::isAccountTransactionQuery() const {
    return this->accountTransactionQuery;
}

keto::asn1::HashHelper URIBlockchainExplorerParser::getHash() const {
    return this->hash;
}

int URIBlockchainExplorerParser::getNumberOfEntries() const {
    return this->numberOfEntries;
}


}
}