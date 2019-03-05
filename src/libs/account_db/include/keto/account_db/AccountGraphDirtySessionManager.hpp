//
// Created by Brett Chaldecott on 2019/03/04.
//

#ifndef KETO_ACCOUNTGRAPHDIRTYSESSIONMANAGER_HPP
#define KETO_ACCOUNTGRAPHDIRTYSESSIONMANAGER_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>


#include "keto/account_db/AccountGraphDirtySession.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace account_db {

class AccountGraphDirtySessionManager;
typedef std::shared_ptr<AccountGraphDirtySessionManager> AccountGraphDirtySessionManagerPtr;

class AccountGraphDirtySessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    AccountGraphDirtySessionManager();
    AccountGraphDirtySessionManager(const AccountGraphDirtySessionManager& orig) = delete;
    virtual ~AccountGraphDirtySessionManager();

    static AccountGraphDirtySessionManagerPtr init();
    static AccountGraphDirtySessionManagerPtr getInstance();
    static void fin();

    AccountGraphDirtySessionPtr getDirtySession(const std::string& name);
    void clearSessions();

private:
    std::condition_variable stateCondition;
    std::mutex classMutex;
    std::map<std::string,AccountGraphDirtySessionPtr> dirtySessions;

};


}
}



#endif //KETO_ACCOUNTGRAPHDIRTYSESSIONMANAGER_HPP
