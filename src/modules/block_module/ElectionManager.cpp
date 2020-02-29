//
// Created by Brett Chaldecott on 2019-08-23.
//

#include <sstream>

#include <botan/hex.h>

#include "keto/block/BlockProducer.hpp"
#include "keto/block/ElectionManager.hpp"
#include "keto/block/Constants.hpp"
#include "keto/block/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/RDFUtils.hpp"

#include "keto/asn1/Constants.hpp"

#include "keto/election_common/ElectionMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPeerMessageProtoHelper.hpp"
#include "keto/election_common/ElectionHelper.hpp"
#include "keto/election_common/SignedElectionHelper.hpp"
#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectNodeHelper.hpp"
#include "keto/election_common/ElectionPublishTangleAccountProtoHelper.hpp"
#include "keto/election_common/ElectionUtils.hpp"
#include "keto/election_common/Constants.hpp"

#include "keto/transaction_common/TransactionTraceBuilder.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"

#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

#include "keto/key_store_utils/Events.hpp"

namespace keto {
namespace block {

static ElectionManagerPtr singleton;

std::string ElectionManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionManager::Elector::Elector(const keto::asn1::HashHelper& account, const std::string& type) : account(account), type(type) {

}

ElectionManager::Elector::~Elector() {

}

keto::asn1::HashHelper ElectionManager::Elector::getAccount() {
    return this->account;
}

std::string ElectionManager::Elector::getType() {
    return this->type;
}

bool ElectionManager::Elector::isSet() {
    if (this->electionResultMessageProtoHelperPtr) {
        return true;
    }
    return false;
}

keto::election_common::ElectionResultMessageProtoHelperPtr ElectionManager::Elector::getElectionResult() {
    return this->electionResultMessageProtoHelperPtr;
}

void ElectionManager::Elector::setElectionResult(
        const keto::election_common::ElectionResultMessageProtoHelperPtr& result) {
    this->electionResultMessageProtoHelperPtr = result;
}

ElectionManager::ElectionManager() : state(ElectionManager::State::PROCESSING) {
    std::shared_ptr<keto::environment::Config> config =
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    if (!config->getVariablesMap().count(Constants::FAUCET_ACCOUNT)) {
        BOOST_THROW_EXCEPTION(keto::block::FaucetNotConfiguredException());
    }
    faucetAccount = keto::asn1::HashHelper(config->getVariablesMap()[Constants::FAUCET_ACCOUNT].as<std::string>(),
                                           keto::common::StringEncoding::HEX);
}

ElectionManager::~ElectionManager() {

}

ElectionManagerPtr ElectionManager::init() {
    return singleton = ElectionManagerPtr(new ElectionManager());
}

void ElectionManager::fin() {
    singleton.reset();
}

ElectionManagerPtr ElectionManager::getInstance() {
    return singleton;
}


keto::event::Event ElectionManager::consensusHeartbeat(const keto::event::Event& event) {
    BlockProducer::State state = BlockProducer::getInstance()->getState();
    KETO_LOG_DEBUG << "[ElectionManager][consensusHeartbeat] block producer [" << state << "]";
    keto::software_consensus::ProtocolHeartbeatMessageHelper protocolHeartbeatMessageHelper(
            keto::server_common::fromEvent<keto::proto::ProtocolHeartbeatMessage>(event));

    KETO_LOG_INFO << "[ElectionManager::consensusHeartbeat] current slot is [" << protocolHeartbeatMessageHelper.getNetworkSlot() <<
                   "][" << protocolHeartbeatMessageHelper.getElectionSlot() << "][" <<
                   protocolHeartbeatMessageHelper.getElectionPublishSlot() << "][" <<
                   protocolHeartbeatMessageHelper.getConfirmationSlot() << "]";

    if (protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getElectionSlot()) {
        KETO_LOG_DEBUG << "[ElectionManager::consensusHeartbeat] clean out the election information : " << state;
        this->accountElectionResult.clear();
        this->responseCount = 0;
        this->nextWindow.clear();
        this->state = ElectionManager::State::ELECT;
        if (state == BlockProducer::State::block_producer) {
            KETO_LOG_INFO << "[ElectionManager::consensusHeartbeat] run the election to choose a new node as state is : " << state;
            invokeElection(keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CLIENT,
                           keto::server_common::Events::PEER_TYPES::CLIENT);
            invokeElection(keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_SERVER,
                           keto::server_common::Events::PEER_TYPES::SERVER);
            KETO_LOG_INFO << "[ElectionManager::consensusHeartbeat] after running the election";
        }

    } else if (protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getElectionPublishSlot()){
        if (state == BlockProducer::State::block_producer) {
            KETO_LOG_INFO << "[BlockProducer::consensusHeartbeat] the election publish has been called";
            publishElection();
            KETO_LOG_INFO << "[BlockProducer::consensusHeartbeat] the publish has been started";
        }
    } else if (protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getConfirmationSlot()){
        KETO_LOG_INFO << "[BlockProducer::consensusHeartbeat] In the confirmation slot : " << state;
        if (state == BlockProducer::State::block_producer) {
            KETO_LOG_INFO << "[BlockProducer::consensusHeartbeat] the confirmation has been called";
            confirmElection();
            KETO_LOG_INFO << "[BlockProducer::consensusHeartbeat] the confirmation has been completed";
        }
    } else {
        this->state = ElectionManager::State::PROCESSING;
    }

    return event;
}



keto::event::Event ElectionManager::electRpcRequest(const keto::event::Event& event) {
    keto::election_common::ElectionPeerMessageProtoHelper electionPeerMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPeerMessage>(event));
    keto::election_common::ElectionPeerMessageProtoHelper responseElectionPeerMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPeerMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ElectionPeerMessage>(
                                    keto::server_common::Events::ROUTER_QUERY::ELECT_ROUTER_PEER,electionPeerMessageProtoHelper))));

    keto::election_common::ElectionHelper electionHelper;
    electionHelper.setAccountHash(responseElectionPeerMessageProtoHelper.getPeer());
    electionHelper.setAcceptedCheck(BlockProducer::getInstance()->getAcceptedCheck().getMsg());

    keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper;
    moduleHashMessageHelper.setHash(responseElectionPeerMessageProtoHelper.getPeer());
    keto::software_consensus::ConsensusMessageHelper consensusMessageHelper(
            keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ModuleHashMessage>(
                                    keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,
                                    moduleHashMessageHelper))));
    electionHelper.setValidateCheck(consensusMessageHelper.getMsg());

    keto::election_common::SignedElectionHelper signedElectionHelper;
    signedElectionHelper.setElectionHelper(electionHelper);
    signedElectionHelper.sign(BlockProducer::getInstance()->getKeyLoader());


    keto::election_common::ElectionResultMessageProtoHelper electionResultMessageProtoHelper;
    electionResultMessageProtoHelper.setSourceAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
    electionResultMessageProtoHelper.setElectionMsg(signedElectionHelper);

    return keto::server_common::toEvent<keto::proto::ElectionResultMessage>(electionResultMessageProtoHelper);
}

keto::event::Event ElectionManager::electRpcResponse(const keto::event::Event& event) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    keto::election_common::ElectionResultMessageProtoHelperPtr electionResultMessageProtoHelperPtr(
            new keto::election_common::ElectionResultMessageProtoHelper(
                    keto::server_common::fromEvent<keto::proto::ElectionResultMessage>(event)));
    if (!this->accountElectionResult.count(electionResultMessageProtoHelperPtr->getSourceAccountHash())) {
        KETO_LOG_DEBUG << "[ElectionManager::electRpcResponse] The account hash is unknown [" <<
            electionResultMessageProtoHelperPtr->getSourceAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
        return event;
    }
    KETO_LOG_DEBUG << "[ElectionManager::electRpcResponse] Add a result for the account [" <<
                   electionResultMessageProtoHelperPtr->getSourceAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
    this->accountElectionResult[electionResultMessageProtoHelperPtr->getSourceAccountHash()]->setElectionResult(electionResultMessageProtoHelperPtr);
    this->responseCount++;
    return event;
}

keto::event::Event ElectionManager::electRpcProcessPublish(const keto::event::Event& event) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));
    KETO_LOG_DEBUG << "[ElectionManager::electRpcProcessPublish] confirm the election [" <<
        electionPublishTangleAccountProtoHelper.getAccount().getHash(keto::common::StringEncoding::HEX) << "][" <<
        Botan::hex_encode(keto::server_common::ServerInfo::getInstance()->getAccountHash(),true) << "]";
    if ((std::vector<uint8_t>)electionPublishTangleAccountProtoHelper.getAccount() ==
        keto::server_common::ServerInfo::getInstance()->getAccountHash()) {
        KETO_LOG_DEBUG << "[ElectionManager::electRpcProcessPublish] set the active tangles to [" <<
            electionPublishTangleAccountProtoHelper.getTangles().size() << "]";
        nextWindow = electionPublishTangleAccountProtoHelper.getTangles();
    }
    this->state = ElectionManager::State::CONFIRMATION;
    return event;
}

keto::event::Event ElectionManager::electRpcProcessConfirmation(const keto::event::Event& event) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));
    KETO_LOG_DEBUG << "[ElectionManager::electRpcProcessConfirmation] confirm the election [" <<
        electionConfirmationHelper.getAccount().getHash(keto::common::StringEncoding::HEX) << "][" <<
        Botan::hex_encode(keto::server_common::ServerInfo::getInstance()->getAccountHash(),true) << "]";
    if ( (electionConfirmationHelper.getAccount() ==
            keto::server_common::ServerInfo::getInstance()->getAccountHash()) &&
            (this->state == ElectionManager::State::CONFIRMATION) &&
            this->nextWindow.size()) {
        KETO_LOG_DEBUG << "[ElectionManager::electRpcProcessConfirmation] this node has been elected set the active tangles [" <<
            this->nextWindow.size() << "]";
        BlockProducer::getInstance()->setActiveTangles(nextWindow);
        this->state = ElectionManager::State::PROCESSING;
        KETO_LOG_INFO << "[ElectionManager::electRpcProcessConfirmation]####################################################################";
        KETO_LOG_INFO << "[ElectionManager::electRpcProcessConfirmation]######## Node is now a producer [" <<
            Botan::hex_encode(keto::server_common::ServerInfo::getInstance()->getAccountHash(),true) << "] ########";
        KETO_LOG_INFO << "[ElectionManager::electRpcProcessConfirmation]####################################################################";

    } else if (!this->nextWindow.size() && this->state == ElectionManager::State::CONFIRMATION) {
        KETO_LOG_DEBUG << "[ElectionManager::electRpcProcessConfirmation] this node is not elected clear it.";
        BlockProducer::getInstance()->setActiveTangles(nextWindow);
        this->state = ElectionManager::State::PROCESSING;
        KETO_LOG_INFO << "[ElectionManager::electRpcProcessConfirmation]####################################################################";
        KETO_LOG_INFO << "[ElectionManager::electRpcProcessConfirmation]######## Node is no longer a producer [" <<
                      Botan::hex_encode(keto::server_common::ServerInfo::getInstance()->getAccountHash(),true) << "] ########";
        KETO_LOG_INFO << "[ElectionManager::electRpcProcessConfirmation]####################################################################";
    }

    return event;
}

void ElectionManager::invokeElection(const std::string& event, const std::string& type) {
    KETO_LOG_ERROR << "[ElectionManager::invokeElection] invoke the election for [" << event << "][" << type << "]";
    keto::election_common::ElectionMessageProtoHelper requestElectionMessageProtoHelper;
    requestElectionMessageProtoHelper.setSource(type);
    keto::election_common::ElectionMessageProtoHelper responseElectionMessageProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionMessage>(
                    keto::server_common::processEvent(
                            keto::server_common::toEvent<keto::proto::ElectionMessage>(
                                    event,requestElectionMessageProtoHelper))));
    KETO_LOG_ERROR << "[ElectionManager::invokeElection] after invoking the election for [" << responseElectionMessageProtoHelper.getAccounts().size() << "]";
    for (keto::asn1::HashHelper hash : responseElectionMessageProtoHelper.getAccounts()) {
        this->accountElectionResult[hash] = ElectorPtr(new Elector(hash,type));
    }
    KETO_LOG_ERROR << "[ElectionManager::invokeElection] after invoking the election for [" << event << "][" << type << "]";
}


void ElectionManager::publishElection() {
    if (!this->responseCount) {
        KETO_LOG_ERROR << "[ElectionManager::publishElection] None of the peers responded this node will remain master until the next election";
        return;
    }
    if (this->responseCount != this->accountElectionResult.size()) {
        KETO_LOG_ERROR << "[ElectionManager::publishElection] Expected [" << this->accountElectionResult.size()
            << "] got [" << this->responseCount << "]";
    }

    std::vector<keto::asn1::HashHelper> tangles = BlockProducer::getInstance()->getActiveTangles();
    std::vector<std::vector<uint8_t>> accounts = this->listAccounts();
    this->electedAccounts.clear();

    while (tangles.size()) {
        KETO_LOG_DEBUG << "[ElectionManager::publishElection] generate the signed elect node";
        keto::election_common::SignedElectNodeHelperPtr signedElectNodeHelperPtr = generateSignedElectedNode(accounts);
        keto::asn1::HashHelper accountHash =
                signedElectNodeHelperPtr->getElectedNode()->getElectedNode()->getElectionHelper()->getAccountHash();
        // push to network
        KETO_LOG_DEBUG << "[ElectionManager::publishElection] get the tangle information associated with elected node ["
            << accountHash.getHash(keto::common::StringEncoding::HEX) << "]";
        keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelperPtr(
                new keto::election_common::ElectionPublishTangleAccountProtoHelper());
        electionPublishTangleAccountProtoHelperPtr->setAccount(accountHash);
        KETO_LOG_DEBUG << "[ElectionManager::publishElection] loop through and add the tangles [" << tangles.size() << "]";
        for(int index = 0;(index < Constants::MAX_TANGLES_TO_ACCOUNT) && (tangles.size()); index++) {
            electionPublishTangleAccountProtoHelperPtr->addTangle(tangles[0]);
            tangles.erase(tangles.begin());
        }
        KETO_LOG_DEBUG << "[ElectionManager::publishElection] set the grow flag tangles [" << tangles.size() << "]";
        if (electionPublishTangleAccountProtoHelperPtr->size() < Constants::MAX_TANGLES_TO_ACCOUNT) {
            electionPublishTangleAccountProtoHelperPtr->setGrowing(true);
        }

        // generate transaction and push
        KETO_LOG_DEBUG << "[ElectionManager::publishElection] publish the results";
        keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_INTERNAL_PUBLISH).
                publish(electionPublishTangleAccountProtoHelperPtr);
        KETO_LOG_DEBUG << "[ElectionManager::publishElection] set the elected accounts";
        this->electedAccounts.insert(accountHash);

        this->generateTransaction(signedElectNodeHelperPtr,tangles);

    }

}

void ElectionManager::confirmElection() {
    if (!this->responseCount) {
        KETO_LOG_INFO << "[ElectionManager::confirmElection] this node will have to remain the master until the next cycle";
        return;
    }

    for (std::vector<std::uint8_t> account : this->electedAccounts) {
        KETO_LOG_INFO << "[ElectionManager::confirmElection] send confirmation for an elected account";
        keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_CONFIRMATION).
        confirmation(account);
    }
    this->electedAccounts.clear();
}


std::vector<std::vector<uint8_t>> ElectionManager::listAccounts() {
    std::vector<std::vector<uint8_t>> keys;
    // loop through keys and add entries that have results
    for (std::map<std::vector<uint8_t>,ElectorPtr>::iterator iter = this->accountElectionResult.begin(); iter != this->accountElectionResult.end();
        iter++) {
        if (iter->second->getElectionResult()) {
            keys.push_back(iter->first);
        }
    }
    return keys;
}


keto::election_common::SignedElectNodeHelperPtr ElectionManager::generateSignedElectedNode(std::vector<std::vector<uint8_t>>& accounts) {
    try {
        // setup the random number generator
        std::default_random_engine stdGenerator;
        stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> distribution(0, accounts.size() - 1);
        // seed
        distribution(stdGenerator);

        // retrieve
        int pos = distribution(stdGenerator);
        std::vector<uint8_t> account = accounts[pos];
        accounts.erase(accounts.begin() + pos);
        ElectorPtr electorPtr = this->accountElectionResult[account];
        keto::asn1::HashHelper electedHash = electorPtr->getElectionResult()->getElectionMsg().getElectionHash();

        // setup the electnode and signed elect node structure
        keto::election_common::ElectNodeHelper electNodeHelper;
        electNodeHelper.setAccountHash(keto::server_common::ServerInfo::getInstance()->getAccountHash());
        electNodeHelper.setElectedNode(*electorPtr->getElectionResult()->getElectionMsg());
        for (std::vector<uint8_t> account: accounts) {
            electNodeHelper.addAlternative(
                    *this->accountElectionResult[account]->getElectionResult()->getElectionMsg());
        }

        electNodeHelper.setAcceptedCheck(BlockProducer::getInstance()->getAcceptedCheck().getMsg());

        keto::software_consensus::ModuleHashMessageHelper moduleHashMessageHelper;
        moduleHashMessageHelper.setHash(electedHash);
        keto::software_consensus::ConsensusMessageHelper consensusMessageHelper(
                keto::server_common::fromEvent<keto::proto::ConsensusMessage>(
                        keto::server_common::processEvent(
                                keto::server_common::toEvent<keto::proto::ModuleHashMessage>(
                                        keto::server_common::Events::GET_SOFTWARE_CONSENSUS_MESSAGE,
                                        moduleHashMessageHelper))));
        electNodeHelper.setValidateCheck(consensusMessageHelper.getMsg());

        // generate the signed object
        keto::election_common::SignedElectNodeHelperPtr signedElectNodeHelperPtr(
                new keto::election_common::SignedElectNodeHelper());
        signedElectNodeHelperPtr->setElectedNode(electNodeHelper);
        signedElectNodeHelperPtr->sign(BlockProducer::getInstance()->getKeyLoader());

        return signedElectNodeHelperPtr;
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "[generateSignedElectedNode] The election failed : " << ex.what();
        KETO_LOG_ERROR << "[generateSignedElectedNode] Cause: " << boost::diagnostic_information(ex,true);
        std::stringstream ss;
        ss << "The election failed : " << boost::diagnostic_information(ex,true);
        BOOST_THROW_EXCEPTION(keto::block::ElectionFailedException(ss.str()));
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[generateSignedElectedNode] The election failed";
        KETO_LOG_ERROR << "[generateSignedElectedNode] Cause: " << boost::diagnostic_information(ex,true);
        std::stringstream ss;
        ss << "The election failed : " << boost::diagnostic_information(ex,true);
        BOOST_THROW_EXCEPTION(keto::block::ElectionFailedException(ss.str()));
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[generateSignedElectedNode] The election failed";
        KETO_LOG_ERROR << "[generateSignedElectedNode] The cause is : " << ex.what();
        std::stringstream ss;
        ss << "The election failed : " << ex.what();
        BOOST_THROW_EXCEPTION(keto::block::ElectionFailedException(ss.str()));
    } catch (...) {
        KETO_LOG_ERROR << "[generateSignedElectedNode] The election failed";
        std::stringstream ss;
        ss << "The election failed";
        BOOST_THROW_EXCEPTION(keto::block::ElectionFailedException(ss.str()));
    }
}


void ElectionManager::generateTransaction(const keto::election_common::SignedElectNodeHelperPtr& signedElectNodeHelperPtr,
                                          const std::vector<keto::asn1::HashHelper>& tangles) {
    try {
        std::shared_ptr<keto::chain_common::TransactionBuilder> transactionPtr =
                keto::chain_common::TransactionBuilder::createTransaction();

        transactionPtr->setSourceAccount(faucetAccount);
        transactionPtr->setTargetAccount(keto::server_common::ServerInfo::getInstance()->getAccountHash());
        transactionPtr->setValue(keto::asn1::NumberHelper());

        // use the facet account as the parent transaction
        transactionPtr->setParent(faucetAccount);

        keto::asn1::RDFModelHelper modelHelper;
        std::string id = signedElectNodeHelperPtr->getElectedHash().getHash(keto::common::StringEncoding::HEX);
        std::string account = Botan::hex_encode(
                keto::server_common::ServerInfo::getInstance()->getAccountHash(), true);
        std::stringstream subject;
        subject << Constants::FaucetRequest::SUBJECT << "/" << id;
        keto::asn1::RDFSubjectHelper subjectHelper(subject.str());
        subjectHelper.addPredicate(
                buildPredicate(Constants::FaucetRequest::ID, keto::asn1::Constants::RDF_NODE::LITERAL,
                               keto::asn1::Constants::RDF_TYPES::STRING, id));
        subjectHelper.addPredicate(
                buildPredicate(Constants::FaucetRequest::ACCOUNT, keto::asn1::Constants::RDF_NODE::LITERAL,
                               keto::asn1::Constants::RDF_TYPES::STRING, account));
        subjectHelper.addPredicate(
                buildPredicate(Constants::FaucetRequest::DATE_TIME, keto::asn1::Constants::RDF_NODE::LITERAL,
                               keto::asn1::Constants::RDF_TYPES::DATE_TIME,
                               keto::server_common::RDFUtils::convertTimeToRDFDateTime(std::time(0))));
        std::stringstream tangleStream;
        for (keto::asn1::HashHelper tangle : tangles) {
            tangleStream << tangle.getHash(keto::common::StringEncoding::HEX) << ",";
        }
        subjectHelper.addPredicate(
                buildPredicate(Constants::FaucetRequest::TANGLES, keto::asn1::Constants::RDF_NODE::LITERAL,
                               keto::asn1::Constants::RDF_TYPES::STRING, tangleStream.str()));
        subjectHelper.addPredicate(
                buildPredicate(Constants::FaucetRequest::PROOF, keto::asn1::Constants::RDF_NODE::LITERAL,
                               keto::asn1::Constants::RDF_TYPES::STRING,
                               Botan::hex_encode((std::vector<uint8_t>) *signedElectNodeHelperPtr, true)));

        // add entries to the model helper to persist with this transaction
        modelHelper.addSubject(subjectHelper);

        keto::asn1::AnyHelper anyModel = modelHelper;
        std::shared_ptr<keto::chain_common::ActionBuilder> actionBuilderPtr =
                keto::chain_common::ActionBuilder::createAction();
        actionBuilderPtr->setModel(anyModel);
        actionBuilderPtr->setContractName(Constants::SYSTEM_CONTRACT::FAUCET_TRANSACTION);
        transactionPtr->addAction(actionBuilderPtr);

        std::shared_ptr<keto::chain_common::SignedTransactionBuilder> signedTransBuild =
                keto::chain_common::SignedTransactionBuilder::createTransaction(
                        BlockProducer::getInstance()->getKeyLoader());
        signedTransBuild->setTransaction(transactionPtr).sign();
        keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr(
                new keto::transaction_common::TransactionWrapperHelper(
                        signedTransBuild->operator SignedTransaction *()));


        transactionWrapperHelperPtr->addTransactionTrace(
                *keto::transaction_common::TransactionTraceBuilder::createTransactionTrace(
                        keto::server_common::ServerInfo::getInstance()->getAccountHash(),
                        BlockProducer::getInstance()->getKeyLoader()));

        keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
                keto::transaction_common::TransactionMessageHelperPtr(
                        new keto::transaction_common::TransactionMessageHelper(
                                transactionWrapperHelperPtr));

        keto::transaction_common::TransactionProtoHelper
                transactionProtoHelper(transactionMessageHelperPtr);


        keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper;
        messageWrapperProtoHelper.setTransaction(transactionProtoHelper);

        keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
        messageWrapper = keto::server_common::fromEvent<keto::proto::MessageWrapper>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                        keto::key_store_utils::Events::TRANSACTION::ENCRYPT_TRANSACTION, messageWrapper)));

        keto::proto::MessageWrapperResponse messageWrapperResponse =
                keto::server_common::fromEvent<keto::proto::MessageWrapperResponse>(
                        keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::MessageWrapper>(
                                keto::server_common::Events::ROUTE_MESSAGE, messageWrapper)));
    } catch (keto::common::Exception& ex) {
        if (ex.getType() == "NoMatchingTangleFound") {
            KETO_LOG_WARNING << "[generateTransaction] The election failed as no tangle information is availble: " << ex.what();
            KETO_LOG_WARNING << "[generateTransaction] Cause: " << boost::diagnostic_information(ex, true);
            KETO_LOG_WARNING << "[generateTransaction] This should only ever  occur with the first network election";
        } else {
            KETO_LOG_ERROR << "[generateTransaction] Faucet transaction failed : " << ex.what();
            KETO_LOG_ERROR << "[generateTransaction] Cause: " << boost::diagnostic_information(ex, true);
        }
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "[generateTransaction] Faucet transaction failed";
        KETO_LOG_ERROR << "[generateTransaction] Cause: " << boost::diagnostic_information(ex,true);
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "[generateTransaction] Faucet transaction failed";
        KETO_LOG_ERROR << "[generateTransaction] The cause is : " << ex.what();
    } catch (...) {
        KETO_LOG_ERROR << "[generateTransaction] Faucet transaction failed for unknown reasons";
    }
}

keto::asn1::RDFPredicateHelper ElectionManager::buildPredicate(const std::string& predicate,
        const std::string& type, const std::string& datatype, const std::string& value) {
    keto::asn1::RDFPredicateHelper predicateHelper(predicate);

    keto::asn1::RDFObjectHelper objectHelper;
    objectHelper.setDataType(datatype).
            setType(type).
            setValue(value);

    predicateHelper.addObject(objectHelper);

    return predicateHelper;
}


}
}

