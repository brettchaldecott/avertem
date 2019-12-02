//
// Created by Brett Chaldecott on 2019/02/18.
//

#include "keto/block/Constants.hpp"
#include "keto/block/Exception.hpp"
#include "keto/block/NetworkFeeManager.hpp"

#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/server_common/ServerInfo.hpp"
#include "keto/environment/Config.hpp"
#include "keto/environment/EnvironmentManager.hpp"

namespace keto {
namespace block {

static NetworkFeeManagerPtr singleton;

std::string NetworkFeeManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


NetworkFeeManager::NetworkFeeManager() {

}

NetworkFeeManager::~NetworkFeeManager() {

}

NetworkFeeManagerPtr NetworkFeeManager::getInstance() {
    return singleton;
}

NetworkFeeManagerPtr NetworkFeeManager::init() {
    return singleton = NetworkFeeManagerPtr(new NetworkFeeManager());
}

void NetworkFeeManager::fin() {
    singleton.reset();
}

void NetworkFeeManager::load() {
    if (keto::server_common::ServerInfo::getInstance()->isMaster()) {
        std::shared_ptr<keto::environment::Config> config =
                keto::environment::EnvironmentManager::getInstance()->getConfig();
        if (!config->getVariablesMap().count(Constants::NETWORK_FEE_RATIO)) {
            BOOST_THROW_EXCEPTION(keto::block::NetworkFeeRatioNotSetException());
        }
        float feeRatio = std::stof(
                config->getVariablesMap()[Constants::NETWORK_FEE_RATIO].as<std::string>());
        this->feeInfoMsgProtoHelperPtr = keto::transaction_common::FeeInfoMsgProtoHelperPtr(
                new keto::transaction_common::FeeInfoMsgProtoHelper(feeRatio));
        this->feeInfoMsgProtoHelperPtr->setMaxFee(Constants::MAX_RUN_TIME / feeRatio);
    }
}

void NetworkFeeManager::clear() {
    this->feeInfoMsgProtoHelperPtr.reset();
}

keto::transaction_common::FeeInfoMsgProtoHelperPtr NetworkFeeManager::getFeeInfo() {
    if (this->feeInfoMsgProtoHelperPtr) {
        return this->feeInfoMsgProtoHelperPtr;
    }  else {
        BOOST_THROW_EXCEPTION(keto::block::NetworkFeeRatioNotSetException());
    }
}

void NetworkFeeManager::setFeeInfo(const keto::transaction_common::FeeInfoMsgProtoHelperPtr& feeInfoMsgProtoHelperPtr) {
    this->feeInfoMsgProtoHelperPtr = feeInfoMsgProtoHelperPtr;
}

keto::event::Event NetworkFeeManager::getNetworkFeeInfo(const keto::event::Event& event) {
    if (this->feeInfoMsgProtoHelperPtr) {
        keto::proto::FeeInfoMsg feeInfoMsg = *getFeeInfo();
        return keto::server_common::toEvent<keto::proto::FeeInfoMsg>(feeInfoMsg);
    } else {
        BOOST_THROW_EXCEPTION(keto::block::NetworkFeeRatioNotSetException());
    }
}

keto::event::Event NetworkFeeManager::setNetworkFeeInfo(const keto::event::Event& event) {
    setFeeInfo(keto::transaction_common::FeeInfoMsgProtoHelperPtr(new keto::transaction_common::FeeInfoMsgProtoHelper(
            keto::server_common::fromEvent<keto::proto::FeeInfoMsg>(event))));
    return event;
}

}
}