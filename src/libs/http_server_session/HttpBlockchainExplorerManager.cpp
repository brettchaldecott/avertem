//
// Created by Brett Chaldecott on 2019-09-19.
//

#include <nlohmann/json.hpp>

#include "keto/server_session/HttpBlockchainExplorerManager.hpp"

#include <string>
#include <iostream>
#include <boost/beast/http/message.hpp>

#include "keto/asn1/HashHelper.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/common/HttpEndPoints.hpp"
#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/server_session/Exception.hpp"

#include "keto/server_common/StringUtils.hpp"

#include "keto/server_session/URIBlockchainExplorerParser.hpp"

#include "keto/chain_query_common/BlockQueryProtoHelper.hpp"
#include "keto/chain_query_common/BlockResultSetProtoHelper.hpp"
#include "keto/chain_query_common/TransactionQueryProtoHelper.hpp"
#include "keto/chain_query_common/TransactionResultSetProtoHelper.hpp"
#include "keto/chain_query_common/ProducerResultProtoHelper.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/DateUtils.hpp"



namespace keto {
namespace server_session {

std::string HttpBlockchainExplorerManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

// due to the size of the json.hpp include this is a local method and not class method.
// this prevents subsequent parsing from processing the large json header file.
nlohmann::json generateTransactionBlock(const std::vector<keto::chain_query_common::TransactionResultProtoHelperPtr>& transactions) {
    nlohmann::json jsonTransactions = {};
    for (keto::chain_query_common::TransactionResultProtoHelperPtr transactionResultProtoHelperPtr: transactions) {
        nlohmann::json changesetHashes = {};
        for (keto::asn1::HashHelper changesetHash : transactionResultProtoHelperPtr->getChangesetHashes()) {
            changesetHashes.push_back({changesetHash.getHash(keto::common::StringEncoding::HEX)});
        }
        nlohmann::json traceHashes = {};
        for (keto::asn1::HashHelper traceHash : transactionResultProtoHelperPtr->getTransactionTraceHashes()) {
            traceHashes.push_back({traceHash.getHash(keto::common::StringEncoding::HEX)});
        }
        jsonTransactions.push_back({
                                           {"hash",transactionResultProtoHelperPtr->getTransactionHashId().getHash(keto::common::StringEncoding::HEX)},
                                           {"created",keto::server_common::DateUtils(
                                                   transactionResultProtoHelperPtr->getCreated()).formatISO8601()},
                                           {"parent",transactionResultProtoHelperPtr->getParentTransactionHashId().getHash(keto::common::StringEncoding::HEX)},
                                           {"source",transactionResultProtoHelperPtr->getSourceAccountHashId().getHash(keto::common::StringEncoding::HEX)},
                                           {"target",transactionResultProtoHelperPtr->getTargetAccountHashId().getHash(keto::common::StringEncoding::HEX)},
                                           {"value",(long)transactionResultProtoHelperPtr->getValue()},
                                           {"changeset",changesetHashes},
                                           {"trace",traceHashes},
                                           {"status",transactionResultProtoHelperPtr->getStatus()}
                                   });
    }
    return jsonTransactions;
}


HttpBlockchainExplorerManager::HttpBlockchainExplorerManager(std::shared_ptr<HttpSessionManager>& httpSessionManagerPtr) :
    httpSessionManagerPtr(httpSessionManagerPtr) {

}

HttpBlockchainExplorerManager::~HttpBlockchainExplorerManager() {

}

std::string HttpBlockchainExplorerManager::processQuery(
        boost::beast::http::request<boost::beast::http::string_body>& req,
        const std::string& body) {
    if (!req.base().count(keto::common::HttpEndPoints::HEADER_SESSION_HASH)) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }
    std::string sessionHash = (const std::string&)req.base().at(keto::common::HttpEndPoints::HEADER_SESSION_HASH);
    keto::asn1::HashHelper sessionHashHelper(
            sessionHash,keto::common::HEX);
    std::vector<uint8_t> vectorHash = keto::crypto::SecureVectorUtils().copyFromSecure(sessionHashHelper);
    if (!httpSessionManagerPtr->isValid(vectorHash)) {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidSessionException());
    }

    boost::beast::string_view path = req.target();
    URIBlockchainExplorerParser uriBlockchainExplorerParser(path.to_string());

    if (uriBlockchainExplorerParser.isBlockQuery()) {
        return this->processBlockQuery(uriBlockchainExplorerParser);
    } else if (uriBlockchainExplorerParser.isTransactionQuery()) {
        return this->processTransactionQuery(uriBlockchainExplorerParser);
    } else if (uriBlockchainExplorerParser.isProducerQuery()) {
        return this->processProducerQuery(uriBlockchainExplorerParser);
    } else {
        BOOST_THROW_EXCEPTION(keto::server_session::InvalidBlockchainExplorerRequest());
    }
}


std::string HttpBlockchainExplorerManager::processBlockQuery(const URIBlockchainExplorerParser& uriBlockchainExplorerParser) {
    keto::chain_query_common::BlockQueryProtoHelper blockQueryProtoHelper;
    if (!uriBlockchainExplorerParser.getHash().empty()) {
        blockQueryProtoHelper.setBlockHashId(uriBlockchainExplorerParser.getHash());
    }
    if (uriBlockchainExplorerParser.getNumberOfEntries() != 0) {
        blockQueryProtoHelper.setNumberOfBlocks(uriBlockchainExplorerParser.getNumberOfEntries());
    }
    keto::chain_query_common::BlockResultSetProtoHelper blockResultSetProtoHelper(
            keto::server_common::fromEvent<keto::proto::BlockResultSet>(
                    keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::BlockQuery>(
                            keto::server_common::Events::BLOCK_QUERY::GET_BLOCKS,blockQueryProtoHelper)))
            );

    nlohmann::json json = {
            {"start_block", blockResultSetProtoHelper.getStartBlockHashId().getHash(keto::common::StringEncoding::HEX)},
            {"end_block", blockResultSetProtoHelper.getEndBlockHashId().getHash(keto::common::StringEncoding::HEX)},
            {"number_of_blocks", blockResultSetProtoHelper.getNumberOfBlocks()},
    };


    nlohmann::json jsonBlocks = {};
    for (keto::chain_query_common::BlockResultProtoHelperPtr blockResultProtoHelperPtr: blockResultSetProtoHelper.getBlockResults()) {
        jsonBlocks.push_back({
                                     {"hash",blockResultProtoHelperPtr->getBlockHashId().getHash(keto::common::StringEncoding::HEX)},
                                     {"tangle",blockResultProtoHelperPtr->getTangleHashId().getHash(keto::common::StringEncoding::HEX)},
                                     {"created",keto::server_common::DateUtils(
                                             blockResultProtoHelperPtr->getCreated()).formatISO8601()},
                                     {"parent",blockResultProtoHelperPtr->getParentBlockHashId().getHash(keto::common::StringEncoding::HEX)},
                                     {"transactions",generateTransactionBlock(blockResultProtoHelperPtr->getTransactions())},
                                     {"accepted",blockResultProtoHelperPtr->getAcceptedHash().getHash(keto::common::StringEncoding::HEX)},
                                     {"validation",blockResultProtoHelperPtr->getValidationHash().getHash(keto::common::StringEncoding::HEX)},
                                     {"merkel",blockResultProtoHelperPtr->getMerkelRoot().getHash(keto::common::StringEncoding::HEX)},
                                     {"height",blockResultProtoHelperPtr->getBlockHeight()}
        });
    }

    json.push_back({"blocks",jsonBlocks});

    return json.dump();
}

std::string HttpBlockchainExplorerManager::processTransactionQuery(const URIBlockchainExplorerParser& uriBlockchainExplorerParser) {
    keto::chain_query_common::TransactionQueryProtoHelper transactionQueryProtoHelper;
    keto::chain_query_common::TransactionResultSetProtoHelper transactionResultSetProtoHelper;
    if (uriBlockchainExplorerParser.isBlockTransactionQuery()) {
        transactionQueryProtoHelper.setBlockHashId(uriBlockchainExplorerParser.getHash());
        transactionResultSetProtoHelper =
                keto::chain_query_common::TransactionResultSetProtoHelper(keto::server_common::fromEvent<keto::proto::TransactionResultSet>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::TransactionQuery>(
                                keto::server_common::Events::BLOCK_QUERY::GET_BLOCK_TRANSACTIONS,transactionQueryProtoHelper))));
    } else if (uriBlockchainExplorerParser.isTransTransactionQuery()) {
        transactionQueryProtoHelper.setTransactionHashId(uriBlockchainExplorerParser.getHash());
        transactionResultSetProtoHelper =
                keto::chain_query_common::TransactionResultSetProtoHelper(keto::server_common::fromEvent<keto::proto::TransactionResultSet>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::TransactionQuery>(
                        keto::server_common::Events::BLOCK_QUERY::GET_TRANSACTION,transactionQueryProtoHelper))));
    } else if (uriBlockchainExplorerParser.isAccountTransactionQuery()) {
        transactionQueryProtoHelper.setAccountHashId(uriBlockchainExplorerParser.getHash());
        transactionResultSetProtoHelper =
                keto::chain_query_common::TransactionResultSetProtoHelper(keto::server_common::fromEvent<keto::proto::TransactionResultSet>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::TransactionQuery>(
                                keto::server_common::Events::BLOCK_QUERY::GET_ACCOUNT_TRANSACTIONS,transactionQueryProtoHelper))));
    }


    nlohmann::json json = {
            {"number_of_transactions", transactionResultSetProtoHelper.getNumberOfTransactions()},
    };


    json.push_back({"transactions",generateTransactionBlock(transactionResultSetProtoHelper.getTransactions())});

    return json.dump();

}

std::string HttpBlockchainExplorerManager::processProducerQuery(const URIBlockchainExplorerParser& uriBlockchainExplorerParser) {
    keto::chain_query_common::ProducerResultProtoHelper producerResultProtoHelper(keto::server_common::fromEvent<keto::proto::ProducerResult>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::ProducerQuery>(
                    keto::server_common::Events::PRODUCER_QUERY::GET_PRODUCER,keto::proto::ProducerQuery()))));

    nlohmann::json json = {
            {"number_of_producers",producerResultProtoHelper.getProducers().size()}
    };
    nlohmann::json jsonProducers = {};
    for (keto::chain_query_common::ProducerInfoResultProtoHelperPtr producerInfoResultProtoHelperPtr : producerResultProtoHelper.getProducers()) {
        nlohmann::json jsonTangles = {};
        for (keto::asn1::HashHelper tangle : producerInfoResultProtoHelperPtr->getTangles()) {
            jsonTangles.push_back({tangle.getHash(keto::common::StringEncoding::HEX)});
        }
        jsonProducers.push_back({
                                     {"account_hash",producerInfoResultProtoHelperPtr->getAccountHashId().getHash(keto::common::StringEncoding::HEX)},
                                     {"tangles",jsonTangles}
                             });
    }

    json.push_back({"producers",jsonProducers});
    return json.dump();
}

}
}