/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ActionHelper.cpp
 * Author: ubuntu
 * 
 * Created on April 4, 2018, 9:12 AM
 */

#include "keto/transaction_common/ActionHelper.hpp"

namespace keto {
namespace transaction_common {

std::string ActionHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ActionHelper::ActionHelper(Action_t* action) : action(action) {
}

ActionHelper::~ActionHelper() {
}

keto::asn1::TimeHelper ActionHelper::getDate() {
    return action->date;
}

keto::asn1::HashHelper ActionHelper::getContract() {
    if (action->contract.size) {
        return action->contract;
    }
    return keto::asn1::HashHelper(); 
}

std::string ActionHelper::getContractName() {
    if (action->contractName.size) {
        return keto::asn1::StringUtils::copyBuffer(action->contractName);
    }
    return std::string;
}

keto::asn1::HashHelper ActionHelper::getParent() {
    return action->parent;
}

keto::asn1::AnyHelper  ActionHelper::getModel() {
    return action->model;
}


}
}
