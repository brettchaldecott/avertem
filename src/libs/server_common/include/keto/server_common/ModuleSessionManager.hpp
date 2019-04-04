//
// Created by Brett Chaldecott on 2019/04/04.
//

#ifndef KETO_MODULESESIONMANAGER_HPP
#define KETO_MODULESESIONMANAGER_HPP

#include <vector>
#include <memory>

#include "keto/obfuscate/MetaString.hpp"

#include "keto/server_common/ModuleSession.hpp"

namespace keto {
namespace server_common {

class ModuleSessionManager;
typedef std::shared_ptr<ModuleSessionManager> ModuleSessionManagerPtr;

class ModuleSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ModuleSessionManager();
    ModuleSessionManager(const ModuleSessionManager& orig) = delete;
    virtual ~ModuleSessionManager();

    static ModuleSessionManagerPtr init();
    static void fin();
    static ModuleSessionManagerPtr getInstance();
    static void addSession(const ModuleSessionPtr& session);



private:
    std::vector<ModuleSessionPtr> sessions;

    void addSessionEntry(const ModuleSessionPtr& session);
};


}
}


#endif //KETO_MODULESESIONMANAGER_HPP
