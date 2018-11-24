/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountService.hpp
 * Author: ubuntu
 *
 * Created on March 6, 2018, 1:41 PM
 */

#ifndef ACCOUNTSERVICE_HPP
#define ACCOUNTSERVICE_HPP

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace account {

class AccountService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    AccountService(const AccountService& orig) = delete;
    virtual ~AccountService();
    
    // account service management methods
    static std::shared_ptr<AccountService> init();
    static void fin();
    static std::shared_ptr<AccountService> getInstance();
    
    // account methods
    keto::event::Event checkAccount(const keto::event::Event& event);
    keto::event::Event getNodeAccountRouting(const keto::event::Event& event);
    keto::event::Event applyTransaction(const keto::event::Event& event);
    keto::event::Event sparqlQuery(const keto::event::Event& event);
    keto::event::Event getContract(const keto::event::Event& event);
    
private:
    AccountService();

};


}
}


#endif /* ACCOUNTSERVICE_HPP */

