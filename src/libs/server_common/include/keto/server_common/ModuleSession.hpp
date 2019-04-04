//
// Created by Brett Chaldecott on 2019/04/04.
//

#ifndef KETO_MODULESESSION_HPP
#define KETO_MODULESESSION_HPP

#include <memory>

namespace keto {
namespace server_common {

class ModuleSession;
typedef std::shared_ptr<ModuleSession> ModuleSessionPtr;

class ModuleSession {
public:

    ModuleSession() {}
    ModuleSession(const ModuleSession& orig) = delete;
    virtual ~ModuleSession() {}

};

}
}

#endif //KETO_MODULESESSION_HPP
