/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountStatementBuilder.hpp
 * Author: ubuntu
 *
 * Created on March 20, 2018, 11:54 AM
 */

#ifndef ACCOUNTSTATEMENTBUILDER_HPP
#define ACCOUNTSTATEMENTBUILDER_HPP

#include <string>
#include <memory>
#include <vector>

#include <librdf.h>
#include <redland.h>
#include <rdf_storage.h>
#include <rdf_model.h>

#include "ChangeSet.h"
#include "Account.pb.h"

#include "keto/account_db/AccountRDFStatement.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace account_db {

class AccountRDFStatementBuilder;
typedef std::shared_ptr<AccountRDFStatementBuilder> AccountRDFStatementBuilderPtr;

class AccountRDFStatementBuilder {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    AccountRDFStatementBuilder(
            const keto::asn1::HashHelper& chainId,
            const keto::asn1::HashHelper& blockId,
            const keto::transaction_common::TransactionWrapperHelperPtr& transactionWrapperHelperPtr,
            bool existingAccount);
    AccountRDFStatementBuilder(const AccountRDFStatementBuilder& orig) = delete;
    virtual ~AccountRDFStatementBuilder();
    
    std::vector<AccountRDFStatementPtr> getStatements();
    
    std::string accountAction();
    keto::proto::AccountInfo getAccountInfo();

    bool isNewChain();
    void setNewChain(bool newChain);
    
private:
    bool newChain;
    keto::asn1::HashHelper chainId;
    keto::asn1::HashHelper blockId;
    keto::asn1::RDFModelHelperPtr rdfModelHelperPtr;
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr;
    std::vector<AccountRDFStatementPtr> statements;
    keto::proto::AccountInfo accountInfo;
    std::string action;

    std::string buildRdfUri(const std::string uri, const std::string id);
    keto::asn1::RDFPredicateHelper buildPredicate(const std::string& predicate,
            const std::string& value, const std::string& type, const std::string& dataType);
    std::string buildTimeString(const std::time_t value);
};


}
}

#endif /* ACCOUNTSTATEMENTBUILDER_HPP */

