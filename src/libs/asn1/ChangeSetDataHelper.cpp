//
// Created by Brett Chaldecott on 2019/02/13.
//

#include "keto/asn1/ChangeSetDataHelper.hpp"
#include "keto/asn1/Exception.hpp"

namespace keto{
namespace asn1 {

std::string ChangeSetDataHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ChangeSetDataHelper::ChangeSetDataHelper(ChangeData_t* changeData) : changeData(changeData) {

}

ChangeSetDataHelper::~ChangeSetDataHelper() {

}

bool ChangeSetDataHelper::isASN1() {
    if (changeData->present == ChangeData_PR_asn1Change) {
        return true;
    }
    return false;
}

keto::asn1::AnyHelper ChangeSetDataHelper::getAny() {
    if (!isASN1()) {
        BOOST_THROW_EXCEPTION(keto::asn1::ChangeDataIsDifferentType());
    }
    return changeData->choice.asn1Change;
}

std::vector<uint8_t> ChangeSetDataHelper::getBytes() {
    if (isASN1()) {
        BOOST_THROW_EXCEPTION(keto::asn1::ChangeDataIsDifferentType());
    }
    std::vector<uint8_t> binary;
    for (int index = 0; index < changeData->choice.binaryChange.size; index++) {
        binary.push_back(changeData->choice.binaryChange.buf[index]);
    }
    return binary;
}

}
}