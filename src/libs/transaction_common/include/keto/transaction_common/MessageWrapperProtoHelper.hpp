/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MessageWrapperProtoHelper.hpp
 * Author: ubuntu
 *
 * Created on May 21, 2018, 2:32 PM
 */

#ifndef MESSAGEWRAPPERPROTOHELPER_HPP
#define MESSAGEWRAPPERPROTOHELPER_HPP

#include "BlockChain.pb.h"
#include "Protocol.pb.h"
#include "google/protobuf/any.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/transaction_common/TransactionProtoHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace transaction_common {


class MessageWrapperProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    MessageWrapperProtoHelper();
    MessageWrapperProtoHelper(const keto::proto::MessageWrapper& wrapper);
    MessageWrapperProtoHelper(const std::string& str);
    MessageWrapperProtoHelper(const keto::proto::Transaction& transaction);
    MessageWrapperProtoHelper(const TransactionProtoHelper& transaction);
    MessageWrapperProtoHelper(const TransactionProtoHelperPtr& transaction);
    MessageWrapperProtoHelper(const MessageWrapperProtoHelper& orig) = default;
    virtual ~MessageWrapperProtoHelper();
    
    MessageWrapperProtoHelper& setAccountHash(const keto::asn1::HashHelper accountHash);
    keto::asn1::HashHelper getAccountHash();
    MessageWrapperProtoHelper& setSessionHash(const std::string& sessionHash);
    keto::asn1::HashHelper getSessionHash();
    MessageWrapperProtoHelper& setSessionHash(const keto::asn1::HashHelper sessionHash);
    MessageWrapperProtoHelper& setOperation(const keto::proto::MessageOperation operation);
    MessageWrapperProtoHelper& setMessageWrapper(const keto::proto::MessageWrapper& wrapper);
    MessageWrapperProtoHelper& setTransaction(const keto::proto::Transaction& transaction);
    MessageWrapperProtoHelper& setTransaction(TransactionProtoHelper& transaction);
    MessageWrapperProtoHelper& setTransaction(const TransactionProtoHelperPtr& transaction);
    
    keto::proto::MessageOperation incrementOperation();
    
    TransactionProtoHelperPtr getTransaction();
    operator keto::proto::MessageWrapper(); 
    
private:
    keto::proto::MessageWrapper wrapper;
};


}
}

#endif /* MESSAGEWRAPPERPROTOHELPER_HPP */

