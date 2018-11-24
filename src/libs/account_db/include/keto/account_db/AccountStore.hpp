/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountStore.hpp
 * Author: ubuntu
 *
 * Created on March 5, 2018, 6:02 AM
 */

#ifndef ACCOUNTSTORE_HPP
#define ACCOUNTSTORE_HPP

#include "Account.pb.h"
#include "Sparql.pb.h"
#include "Contract.pb.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/rocks_db/DBManager.hpp"
#include "keto/account_db/AccountResourceManager.hpp"
#include "keto/account_db/AccountGraphStoreManager.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/account_db/AccountRDFStatementBuilder.hpp"
#include "keto/router_utils/RpcPeerHelper.hpp"
#include "keto/router_utils/PushAccountHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace account_db {


class AccountStore {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    AccountStore(const AccountStore& orig) = delete;
    virtual ~AccountStore();
    
    static std::shared_ptr<AccountStore> init();
    static void fin();
    static std::shared_ptr<AccountStore> getInstance();
    
    bool getAccountInfo(const keto::asn1::HashHelper& accountHash,
        keto::proto::AccountInfo& result);
    void applyTransaction(
        const keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelper);
    void sparqlQuery(
        const keto::proto::AccountInfo& accountInfo,
        keto::proto::SparqlQuery& sparlQuery);
    void getContract(
        const keto::proto::AccountInfo& accountInfo,
        keto::proto::ContractMessage& contractMessage);
    void getNodeAccountRouting(const keto::asn1::HashHelper& accountHash,
            keto::router_utils::RpcPeerHelper& rpcPeerHelper);
    
private:
    std::shared_ptr<keto::rocks_db::DBManager> dbManagerPtr;
    AccountGraphStoreManagerPtr accountGraphStoreManagerPtr;
    AccountResourceManagerPtr accountResourceManagerPtr;
    
    AccountStore();
    
    void createAccount(
            const keto::asn1::HashHelper& accountHash,
            const keto::transaction_common::TransactionMessageHelperPtr& transactionMessageHelper,
            AccountRDFStatementBuilderPtr accountRDFStatementBuilder,
            keto::proto::AccountInfo& accountInfo);
    
    void setAccountInfo(const keto::asn1::HashHelper& accountHash,
            keto::proto::AccountInfo& accountInfo);
    
    void buildNodeAccountRouting(const std::string& accountHash,
            const std::vector<keto::proto::AccountInfo>& accountInfoVector,
            keto::router_utils::PushAccountHelper& pushAccountHelper);
};


}
}

#endif /* ACCOUNTSTORE_HPP */

