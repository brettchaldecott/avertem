/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignedTransactionBuilder.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 31, 2018, 7:24 AM
 */

#ifndef SIGNEDTRANSACTIONBUILDER_HPP
#define SIGNEDTRANSACTIONBUILDER_HPP

#include <memory>
#include <vector>
#include <iostream>
#include <cerrno>
#include <cstring>

#include "Transaction.h"
#include "SignedTransaction.h"
#include "keto/crypto/Containers.hpp"
#include "keto/asn1/PrivateKeyHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/KeyHelper.hpp"
#include "keto/chain_common/ActionBuilder.hpp"
#include "TransactionBuilder.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace chain_common {

class SignedTransactionBuilder {
public:
    
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    
    SignedTransactionBuilder(const SignedTransactionBuilder& orig) = delete;
    virtual ~SignedTransactionBuilder();
    
    static std::shared_ptr<SignedTransactionBuilder> createTransaction(
        const keto::asn1::PrivateKeyHelper& privateKeyHelper);
    
    
    SignedTransactionBuilder& setTransaction(
        const std::shared_ptr<keto::chain_common::TransactionBuilder>& transactionBuilder);
    
    keto::asn1::HashHelper getSourceAccount();
    keto::asn1::HashHelper getHash();
    keto::asn1::SignatureHelper getSignature();
    
    void sign();
    
    operator std::vector<uint8_t>&();
    
    operator uint8_t*();
    
    size_t size();
    
    operator SignedTransaction*();
    
private:
    SignedTransaction* signedTransaction;
    keto::asn1::PrivateKeyHelper privateKeyHelper;
    std::shared_ptr<keto::asn1::SerializationHelper<SignedTransaction>> serializationHelperPtr;
    
    SignedTransactionBuilder(const keto::asn1::PrivateKeyHelper& privateKeyHelper);
    
    void serializeTransaction();
};


}
}


#endif /* SIGNEDTRANSACTIONBUILDER_HPP */

