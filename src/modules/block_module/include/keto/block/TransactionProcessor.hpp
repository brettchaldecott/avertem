/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionProcessor.hpp
 * Author: ubuntu
 *
 * Created on April 2, 2018, 9:00 AM
 */

#ifndef TRANSACTIONPROCESSOR_HPP
#define TRANSACTIONPROCESSOR_HPP

#include <memory>
#include <string>

#include "BlockChain.pb.h"

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace block {

class TransactionProcessor;
typedef std::shared_ptr<TransactionProcessor> TransactionProcessorPtr;

class TransactionProcessor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    TransactionProcessor();
    TransactionProcessor(const TransactionProcessor& orig) = delete;
    virtual ~TransactionProcessor();
    
    static TransactionProcessorPtr init();
    static void fin();
    static TransactionProcessorPtr getInstance();
    
    keto::proto::Transaction processTransaction(keto::proto::Transaction& transaction);
    
private:
    
};


}
}

#endif /* TRANSACTIONPROCESSOR_HPP */

