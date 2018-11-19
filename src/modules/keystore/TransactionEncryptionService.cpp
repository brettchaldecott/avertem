/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionEncryptionManager.cpp
 * Author: brettchaldecott
 * 
 * Created on 15 November 2018, 8:51 PM
 */

#include "keto/keystore/TransactionEncryptionService.hpp"


namespace keto {
namespace keystore {

static TransactionEncryptionServicePtr singleton;
    
std::string TransactionEncryptionService::getSourceVersion() {
    return return OBFUSCATED("$Id$");
}

TransactionEncryptionService::TransactionEncryptionService() {
    
}

TransactionEncryptionService::~TransactionEncryptionService() {
    
}

TransactionEncryptionServicePtr TransactionEncryptionService::init() {
    return singleton = TransactionEncryptionServicePtr(new TransactionEncryptionService());
}


void TransactionEncryptionService::fin() {
    singleton.reset();
}

TransactionEncryptionServicePtr TransactionEncryptionService::getInstance() {
    return singleton;
}


keto::event::Event TransactionEncryptionService::reencryptTransaction(const keto::event::Event& event) {
    
    return event;
}

keto::event::Event TransactionEncryptionService::encryptTransaction(const keto::event::Event& event) {
    return event;
}

keto::event::Event TransactionEncryptionService::decryptTransaction(const keto::event::Event& event) {
    return event;
}


}
}