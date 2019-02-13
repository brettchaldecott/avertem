/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ChangeSetHelper.hpp
 * Author: ubuntu
 *
 * Created on March 13, 2018, 10:54 AM
 */

#ifndef CHANGESETHELPER_HPP
#define CHANGESETHELPER_HPP

#include <string>
#include <memory>

#include "Status.h"
#include "ChangeData.h"
#include "ChangeSet.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/ChangeSetDataHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace asn1 {

class ChangeSetHelper;
typedef std::shared_ptr<ChangeSetHelper> ChangeSetHelperPtr;

class ChangeSetHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();


    ChangeSetHelper();
    ChangeSetHelper(ChangeSet_t* changeSet);
    ChangeSetHelper(const HashHelper& transactionHash,const HashHelper& accountHash);
    ChangeSetHelper(const ChangeSetHelper& orig) = default;
    virtual ~ChangeSetHelper();
    
    ChangeSetHelper& setTransactionHash(const HashHelper& transactionHash);
    HashHelper getTransactionHash();
    ChangeSetHelper& setAccountHash(const HashHelper& accountHash);
    HashHelper getAccountHash();
    ChangeSetHelper& setStatus(const Status_t status);
    Status_t getStatus();
    ChangeSetHelper& addChange(const ANY_t& change);

    std::vector<ChangeSetDataHelperPtr> getChanges();
    
    operator ChangeSet_t*();
    operator ANY_t*();
    
private:
    bool own;
    ChangeSet_t* changeSet;
    HashHelper transactionHash;
    HashHelper accountHash;
    Status_t status;
    std::vector<ChangeData_t*> changes;
};


}
}


#endif /* CHANGESETHELPER_HPP */

