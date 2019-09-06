//
// Created by Brett Chaldecott on 2019-08-23.
//

#include "keto/block/BlockProducer.hpp"
#include "keto/block/ElectionManager.hpp"
#include "keto/block/Constants.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

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

#include "keto/software_consensus/ModuleConsensusHelper.hpp"
#include "keto/software_consensus/ModuleHashMessageHelper.hpp"

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
    std::cout << "[BlockProducer][consensusHeartbeat] block producer [" << state << "]" << std::endl;
    keto::software_consensus::ProtocolHeartbeatMessageHelper protocolHeartbeatMessageHelper(
            keto::server_common::fromEvent<keto::proto::ProtocolHeartbeatMessage>(event));

    if (state == BlockProducer::State::block_producer &&
        protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getElectionSlot()) {
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] elect a new block producer";
        accountElectionResult.clear();
        this->responseCount = 0;
        invokeElection(keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_CLIENT, keto::server_common::Events::PEER_TYPES::CLIENT);
        invokeElection(keto::server_common::Events::BLOCK_PRODUCER_ELECTION::ELECT_RPC_SERVER, keto::server_common::Events::PEER_TYPES::SERVER);
        this->state = ElectionManager::State::ELECT;
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] election is now running";
    } else if (state == BlockProducer::State::block_producer &&
               protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getElectionPublishSlot()){
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] the election publish has been called";
        publishElection();
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] the publish has been started";
    } else if (state == BlockProducer::State::block_producer &&
               protocolHeartbeatMessageHelper.getNetworkSlot() == protocolHeartbeatMessageHelper.getConfirmationSlot()){
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] the confirmation has been called";
        confirmElection();
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] the confirmation has been completed";
    } else {
        KETO_LOG_DEBUG << "[BlockProducer::consensusHeartbeat] ignore the slot [" << protocolHeartbeatMessageHelper.getNetworkSlot() <<
                       "][" << protocolHeartbeatMessageHelper.getElectionSlot() << "][" <<
                       protocolHeartbeatMessageHelper.getElectionPublishSlot() << "][" <<
                       protocolHeartbeatMessageHelper.getConfirmationSlot() << "]";

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
    std::lock_guard<std::mutex> guard(classMutex);
    keto::election_common::ElectionResultMessageProtoHelperPtr electionResultMessageProtoHelperPtr(
            new keto::election_common::ElectionResultMessageProtoHelper(
                    keto::server_common::fromEvent<keto::proto::ElectionResultMessage>(event)));
    if (this->accountElectionResult.count(electionResultMessageProtoHelperPtr->getSourceAccountHash())) {
        KETO_LOG_DEBUG << "[ElectionManager::electRpcResponse] The account hash is unknown [" <<
            electionResultMessageProtoHelperPtr->getSourceAccountHash().getHash(keto::common::StringEncoding::HEX) << "]";
        return event;
    }
    this->accountElectionResult[electionResultMessageProtoHelperPtr->getSourceAccountHash()]->setElectionResult(electionResultMessageProtoHelperPtr);
    this->responseCount++;
    return event;
}

keto::event::Event ElectionManager::electRpcProcessPublish(const keto::event::Event& event) {
    std::lock_guard<std::mutex> guard(classMutex);
    keto::election_common::ElectionPublishTangleAccountProtoHelper electionPublishTangleAccountProtoHelper(
            keto::server_common::fromEvent<keto::proto::ElectionPublishTangleAccount>(event));
    if ((std::vector<uint8_t>)electionPublishTangleAccountProtoHelper.getAccount() ==
        keto::server_common::ServerInfo::getInstance()->getAccountHash()) {
        nextWindow = electionPublishTangleAccountProtoHelper.getTangles();
    }
    this->state = ElectionManager::State::CONFIRMATION;
    return event;
}

keto::event::Event ElectionManager::electRpcProcessConfirmation(const keto::event::Event& event) {
    std::lock_guard<std::mutex> guard(classMutex);
    keto::election_common::ElectionConfirmationHelper electionConfirmationHelper(
            keto::server_common::fromEvent<keto::proto::ElectionConfirmation>(event));
    if ( (electionConfirmationHelper.getAccount() ==
            keto::server_common::ServerInfo::getInstance()->getAccountHash()) &&
            (this->state == ElectionManager::State::CONFIRMATION) ) {
        BlockProducer::getInstance()->setActiveTangles(nextWindow);
        nextWindow.clear();
    }
    this->state = ElectionManager::State::PROCESSING;
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
        keto::election_common::SignedElectNodeHelperPtr signedElectNodeHelperPtr = generateSignedElectedNode(accounts);
        keto::asn1::HashHelper accountHash =
                signedElectNodeHelperPtr->getElectedNode()->getElectedNode()->getElectionHelper()->getAccountHash();
        // push to network
        keto::election_common::ElectionPublishTangleAccountProtoHelperPtr electionPublishTangleAccountProtoHelperPtr(
                new keto::election_common::ElectionPublishTangleAccountProtoHelper());
        electionPublishTangleAccountProtoHelperPtr->setAccount(accountHash);
        for(int index = 0;(index < Constants::MAX_TANGLES_TO_ACCOUNT) && (tangles.size()); index++) {
            electionPublishTangleAccountProtoHelperPtr->addTangle(tangles[0]);
            tangles.erase(tangles.begin());
        }
        if (!(electionPublishTangleAccountProtoHelperPtr->size() >= Constants::MAX_TANGLES_TO_ACCOUNT)) {
            electionPublishTangleAccountProtoHelperPtr->setGrowing(false);
        }

        // generate transaction and push
        keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_INTERNAL_PUBLISH).
                publish(electionPublishTangleAccountProtoHelperPtr);

        this->electedAccounts.insert(accountHash);
    }

}

void ElectionManager::confirmElection() {
    if (!this->responseCount) {
        KETO_LOG_ERROR << "[ElectionManager::confirmElection] this node will have to remain the master until the next cycle";
        return;
    }

    for (std::vector<std::uint8_t> account : this->electedAccounts) {
            keto::election_common::ElectionUtils(keto::election_common::Constants::ELECTION_PROCESS_CONFIRMATION).
                    confirmation(account);
    }
    this->electedAccounts.clear();
}


std::vector<std::vector<uint8_t>> ElectionManager::listAccounts() {
    std::vector<std::vector<uint8_t>> keys;
    std::transform(
            this->accountElectionResult.begin(),
            this->accountElectionResult.end(),
            std::back_inserter(keys),
            [](const std::map<std::vector<uint8_t>,ElectorPtr>::value_type
               &pair){return pair.first;});
    return keys;
}


keto::election_common::SignedElectNodeHelperPtr ElectionManager::generateSignedElectedNode(std::vector<std::vector<uint8_t>>& accounts) {
    // setup the random number generator
    std::default_random_engine stdGenerator;
    stdGenerator.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,accounts.size()-1);
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
    electNodeHelper.setElectedNode(*electorPtr->getElectionResult()->getElectionMsg());
    for (std::vector<uint8_t> account: accounts) {
        electNodeHelper.addAlternative(*this->accountElectionResult[account]->getElectionResult()->getElectionMsg());
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
}

}
}

