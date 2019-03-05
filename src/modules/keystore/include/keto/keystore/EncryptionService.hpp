//
// Created by Brett Chaldecott on 2019/03/05.
//

#ifndef KETO_ENCRYPTIONSERVICE_HPP
#define KETO_ENCRYPTIONSERVICE_HPP

#include <string>
#include <memory>

#include "keto/event/Event.hpp"
#include "keto/keystore/SessionKeyManager.hpp"


namespace keto {
namespace keystore {


class EncryptionService;
typedef std::shared_ptr<EncryptionService> EncryptionServicePtr;

class EncryptionService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    EncryptionService();
    EncryptionService(const EncryptionService& orig) = delete;
    virtual ~EncryptionService();

    static EncryptionServicePtr init();
    static void fin();
    static EncryptionServicePtr getInstance();

    keto::event::Event encryptAsn1(const keto::event::Event& event);
    keto::event::Event decryptAsn1(const keto::event::Event& event);


private:

};


}
}


#endif //KETO_ENCRYPTIONSERVICE_HPP
