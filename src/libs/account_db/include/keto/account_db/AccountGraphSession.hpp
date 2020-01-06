/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountGraphSession.hpp
 * Author: ubuntu
 *
 * Created on March 28, 2018, 6:00 AM
 */

#ifndef ACCOUNTGRAPHSESSION_HPP
#define ACCOUNTGRAPHSESSION_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>


#include <librdf.h>
#include <redland.h>
#include <rdf_storage.h>
#include <rdf_model.h>

#include "keto/account_db/AccountGraphStore.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace account_db {

class AccountGraphSession;
typedef std::shared_ptr<AccountGraphSession> AccountGraphSessionPtr;
typedef std::map<std::string,std::string> ResultMap;
typedef std::vector<ResultMap> ResultVectorMap;
    
class AccountGraphSession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();
    
    friend class AccountResource;
    AccountGraphSession(const AccountGraphSession& orig) = delete;
    virtual ~AccountGraphSession();

    void persistDirty(keto::asn1::RDFSubjectHelperPtr& subject);
    void persist(keto::asn1::RDFSubjectHelperPtr& subject);
    void remove(keto::asn1::RDFSubjectHelperPtr& subject);
    std::string query(const std::string& query, const std::vector<uint8_t>& accountHash);
    ResultVectorMap executeDirtyQuery(const std::string& queryStr, const std::vector<uint8_t>& accountHash);
    ResultVectorMap executeQuery(const std::string& queryStr, const std::vector<uint8_t>& accountHash);
    ResultVectorMap executeQueryInternal(const std::string& queryStr);
    
protected:
    AccountGraphSession(const AccountGraphStorePtr& accountGraphStore);
    
    void commit();
    void rollback();
    
private:
    AccountGraphStorePtr accountGraphStore;

    // active transaction models
    librdf_world* world;
    librdf_storage* addStorage;
    librdf_storage* removeStorage;
    //librdf_model* searchModel;
    librdf_model* addModel;
    librdf_model* removeModel;
    
    // transaction methods
    
};

        
}
}

#endif /* ACCOUNTGRAPHSESSION_HPP */

