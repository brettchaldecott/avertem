/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TransactionEncryptionManager.hpp
 * Author: brettchaldecott
 *
 * Created on 15 November 2018, 8:51 PM
 */

#ifndef TRANSACTIONENCRYPTIONSERVICE_HPP
#define TRANSACTIONENCRYPTIONSERVICE_HPP

#include <string>
#include <memory>


#include "keto/keystore/SessionKeyManager.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace keystore {
    
class TransactionEncryptionService;
typedef std::shared_ptr<TransactionEncryptionService> TransactionEncryptionServicePtr;

class TransactionEncryptionService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    TransactionEncryptionService();
    TransactionEncryptionService(const TransactionEncryptionService& orig) = delete;
    virtual ~TransactionEncryptionService();
    
    static TransactionEncryptionServicePtr init();
    static void fin();
    static TransactionEncryptionServicePtr getInstance();
    
    
    keto::event::Event reencryptTransaction(const keto::event::Event& event);
    keto::event::Event encryptTransaction(const keto::event::Event& event);
    keto::event::Event decryptTransaction(const keto::event::Event& event);
    
    
private:
    
    
    
};


}
}


#endif /* TRANSACTIONENCRYPTIONMANAGER_HPP */

