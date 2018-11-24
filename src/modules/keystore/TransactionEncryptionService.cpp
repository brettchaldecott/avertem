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

#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/keystore/TransactionEncryptionService.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"
#include "keto/transaction_common/MessageWrapperProtoHelper.hpp"
#include "keto/server_common/EventUtils.hpp"
#include "keto/keystore/KeyStoreService.hpp"
#include "keto/keystore/Exception.hpp"
#include "keto/keystore/KeyStoreReEncryptTransactionMessageHelper.hpp"



namespace keto {
namespace keystore {

static TransactionEncryptionServicePtr singleton;
    
std::string TransactionEncryptionService::getSourceVersion() {
    return OBFUSCATED("$Id$");
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
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
        keto::server_common::fromEvent<keto::proto::MessageWrapper>(event));
    
    keto::asn1::HashHelper sessionHash = messageWrapperProtoHelper.getSessionHash();
    SessionKeyManagerPtr sessionKeyManagerPtr = KeyStoreService::getInstance()->getSessionKeyManager();
    if (!sessionKeyManagerPtr->isSessionKeyValid(sessionHash)) {
        BOOST_THROW_EXCEPTION(keto::keystore::InvalidSessionKeyException(
                    "The sessionkey is invalid."));
    }
    
    // retrieve the private key
    std::cout << "The decrptor has been called" << std::endl;
    KeyStoreReEncryptTransactionMessageHelper decryptor(
            sessionKeyManagerPtr->getPrivateKey(sessionHash));
    
    
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper()->decryptMessage(decryptor);
    
    
    
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
