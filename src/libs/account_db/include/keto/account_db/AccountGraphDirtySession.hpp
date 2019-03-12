//
// Created by Brett Chaldecott on 2019/03/04.
//

#ifndef KETO_ACCOUNTGRAPHDIRTYSESSION_HPP
#define KETO_ACCOUNTGRAPHDIRTYSESSION_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>

#include <librdf.h>
#include <redland.h>
#include <rdf_storage.h>
#include <rdf_model.h>

#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace account_db {

class AccountGraphDirtySession;
typedef std::shared_ptr <AccountGraphDirtySession> AccountGraphDirtySessionPtr;


class AccountGraphDirtySession {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class AccountGraphDirtySessionScope {
    public:
        AccountGraphDirtySessionScope(const std::string& name, librdf_model* model);
        AccountGraphDirtySessionScope(const AccountGraphDirtySessionScope& orig) = delete;
        virtual ~AccountGraphDirtySessionScope();
    private:
        std::string name;
        librdf_model* model;
    };

    friend class AccountGraphSession;

    AccountGraphDirtySession(const std::string& name);
    AccountGraphDirtySession(const AccountGraphDirtySession& orig) = delete;
    virtual ~AccountGraphDirtySession();

    std::string getName();
    void persistDirty(keto::asn1::RDFSubjectHelperPtr& subject);

protected:
    librdf_model* getDirtyModel();
    void addDirtyNodesToContext(librdf_model* model);
    void removeDirtyNodesFromContext(librdf_model* model);

private:
    std::string name;
    librdf_world* world;
    librdf_storage* storage;
    librdf_model* model;


    librdf_node* buildDirtyContext();
};

}
}

#endif //KETO_ACCOUNTGRAPHDIRTYSESSION_HPP
