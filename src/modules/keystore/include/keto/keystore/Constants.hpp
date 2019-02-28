//
//  Constants.hpp
//  KETO
//
//  Created by Brett Chaldecott on 2018/11/23.
//

#ifndef Constants_h
#define Constants_h

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace keystore {

class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static constexpr const char* MODULE_NAME = "KETO_KEY_STORE";
    static constexpr const char* ENCRYPTION_PADDING = "EME1(SHA-256)";
    static constexpr const char* IS_MASTER = "is_master";
    static constexpr const char* IS_MASTER_TRUE = "TRUE";
    static constexpr const char* MASTER_PASSWORD = "master_password";
    static constexpr const char* FALSE_MASTER_PASSWORD = "master_password";
    static constexpr const char* IS_NETWORK_SESSION_GENERATOR = "is_network_session_generator";
    static constexpr const char* IS_NETWORK_SESSION_GENERATOR_TRUE = "TRUE";
    static constexpr const char* NETWORK_SESSION_GENERATOR_KEYS = "network_session_generator_keys";
    static constexpr const int NETWORK_SESSION_GENERATOR_KEYS_DEFAULT = 20;
    static constexpr const int ONION_LEVELS = 3;

    static const char* KEY_HASH;
    static const char* PRIVATE_KEY;
    static const char* PUBLIC_KEY;
    static const char* KEY_ACTIVE;

    static const char* MASTER_PRIVATE_KEY;
    static const char* MASTER_PUBLIC_KEY;

    static const char* NETWORK_KEY_HASH;
    static const char* NETWORK_KEY_ACTIVE;


    class KEY_STORE_DB {
    public:
        static constexpr const char* KEY_STORE_MASTER_ENTRY = "KEY_STORE_MASTER_ENTRY";
        static constexpr const char* KEY_STORE_WRAPPER_ENTRY = "KEY_STORE_WRAPPER_ENTRY";
        static constexpr const int KEY_STORE_WRAPPER_SIZE = 8;
        static constexpr const char* KEY_STORE_WRAPPER_INDEX = "KEY_STORE_WRAPPER_INDEX";
    };


    
private:
};

}
}


#endif /* Constants_h */
