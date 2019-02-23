//
// Created by Brett Chaldecott on 2019/02/18.
//

#include "keto/server_common/VectorUtils.hpp"
#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"
#include "keto/transaction_common/Constants.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace transaction_common {

std::string FeeInfoMsgProtoHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}


FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper() {
    this->feeInfoMsg.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);

}

FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper(const float fee) {
    this->feeInfoMsg.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    this->feeInfoMsg.set_fee_ratio(fee);
    google::protobuf::Timestamp expiryTime;
    expiryTime.set_seconds(time(0) + Constants::DEFAULT_EXIRY_DURATION);
    expiryTime.set_nanos(0);
    *this->feeInfoMsg.mutable_expiry_time() = expiryTime;
}

FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper(const float fee, const long expiryDuration) {
    this->feeInfoMsg.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    this->feeInfoMsg.set_fee_ratio(fee);

    google::protobuf::Timestamp expiryTime;
    expiryTime.set_seconds(time(0) + expiryDuration);
    expiryTime.set_nanos(0);
    *this->feeInfoMsg.mutable_expiry_time() = expiryTime;
}

FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper(const float fee, const long expiryDuration, const long maxFee) {
    this->feeInfoMsg.set_version(keto::common::MetaInfo::PROTOCOL_VERSION);
    this->feeInfoMsg.set_fee_ratio(fee);
    this->feeInfoMsg.set_max_fee(maxFee);

    google::protobuf::Timestamp expiryTime;
    expiryTime.set_seconds(time(0) + expiryDuration);
    expiryTime.set_nanos(0);
    *this->feeInfoMsg.mutable_expiry_time() = expiryTime;
}

FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper(const keto::proto::FeeInfoMsg& feeInfoMsg) : feeInfoMsg(feeInfoMsg) {

}

FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper(const std::string& msg) {
    this->feeInfoMsg.ParseFromString(msg);
}

FeeInfoMsgProtoHelper::FeeInfoMsgProtoHelper(const std::vector<uint8_t>& msg) {
    this->feeInfoMsg.ParseFromString(keto::server_common::VectorUtils().copyVectorToString(msg));
}

FeeInfoMsgProtoHelper::~FeeInfoMsgProtoHelper() {
}


FeeInfoMsgProtoHelper& FeeInfoMsgProtoHelper::setFeeRatio(const float fee) {
    this->feeInfoMsg.set_fee_ratio(fee);
    return *this;
}

float FeeInfoMsgProtoHelper::getFeeRatio() {
    return this->feeInfoMsg.fee_ratio();
}

FeeInfoMsgProtoHelper& FeeInfoMsgProtoHelper::setMaxFee(const long maxFee) {
    this->feeInfoMsg.set_max_fee(maxFee);
    return *this;
}

long FeeInfoMsgProtoHelper::getMaxFee() {
    return this->feeInfoMsg.max_fee();
}

std::time_t FeeInfoMsgProtoHelper::getExpiryTime() {
    return this->feeInfoMsg.expiry_time().seconds();
}

FeeInfoMsgProtoHelper& FeeInfoMsgProtoHelper::setExpiryTime(const std::time_t& expiry) {
    google::protobuf::Timestamp expiryTime;
    expiryTime.set_seconds(expiry);
    expiryTime.set_nanos(0);
    *this->feeInfoMsg.mutable_expiry_time() = expiryTime;
    return *this;
}

bool FeeInfoMsgProtoHelper::isExpired() {
    std::time_t currentTime = time(0);
    if (this->feeInfoMsg.expiry_time().seconds() < currentTime) {
        return true;
    }
    return false;
}

FeeInfoMsgProtoHelper::operator keto::proto::FeeInfoMsg() {
    return this->feeInfoMsg;
}

FeeInfoMsgProtoHelper::operator std::string() {
    std::string value;
    feeInfoMsg.SerializeToString(&value);
    return value;
}


}
}
