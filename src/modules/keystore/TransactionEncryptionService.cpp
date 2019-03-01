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
#include "keto/keystore/NetworkSessionKeyTransactionEncryptionHandler.hpp"



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
    //std::cout << "The decrptor has been called" << std::endl;
    KeyStoreReEncryptTransactionMessageHelper decryptor(
            sessionKeyManagerPtr->getPrivateKey(sessionHash));
    NetworkSessionKeyTransactionEncryptionHandler encryptor;
    
    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper()->decryptMessage(decryptor);

    //std::cout << "Re-incrypt the transaction using the network session key transaction encryptor" << std::endl;
    messageWrapperProtoHelper.setTransaction(
            messageWrapperProtoHelper.getTransaction()->setTransaction(transactionMessageHelperPtr,encryptor));

    //std::cout << "Return the encrypted transactions" << std::endl;
    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    return keto::server_common::toEvent<keto::proto::MessageWrapper>(messageWrapper);
}

keto::event::Event TransactionEncryptionService::encryptTransaction(const keto::event::Event& event) {
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event));

    // retrieve the private key
    NetworkSessionKeyTransactionEncryptionHandler encryptor;

    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper();

    //std::cout << "Encrypt the transaction using the network session key transaction encryptor" << std::endl;
    messageWrapperProtoHelper.setTransaction(
            messageWrapperProtoHelper.getTransaction()->setTransaction(transactionMessageHelperPtr,encryptor));

    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    return keto::server_common::toEvent<keto::proto::MessageWrapper>(messageWrapper);
}

keto::event::Event TransactionEncryptionService::decryptTransaction(const keto::event::Event& event) {
    keto::transaction_common::MessageWrapperProtoHelper messageWrapperProtoHelper(
            keto::server_common::fromEvent<keto::proto::MessageWrapper>(event));

    // retrieve the private key
    //std::cout << "Decrypt the transaction" << std::endl;
    NetworkSessionKeyTransactionEncryptionHandler decryptor;

    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr =
            messageWrapperProtoHelper.getTransaction()->getTransactionMessageHelper()->decryptMessage(decryptor);

    //std::cout << "Set the transaction after decryption" << std::endl;
    messageWrapperProtoHelper.setTransaction(
            messageWrapperProtoHelper.getTransaction()->setTransaction(transactionMessageHelperPtr));

    keto::proto::MessageWrapper messageWrapper = messageWrapperProtoHelper;
    return keto::server_common::toEvent<keto::proto::MessageWrapper>(messageWrapper);
}


}
}
