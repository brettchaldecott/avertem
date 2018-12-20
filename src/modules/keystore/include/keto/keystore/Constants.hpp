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
    
    static constexpr const char* ENCRYPTION_PADDING = "EME1(SHA-256)";
    static constexpr const char* IS_MASTER = "is_master";
    static constexpr const char* IS_MASTER_TRUE = "TRUE";
    static const char* PRIVATE_KEY;
    static const char* PUBLIC_KEY;

    class KEY_STORE_DB {
    public:
        static constexpr const char* KEY_STORE_MASTER_ENTRY = "KEY_STORE_MASTER_ENTRY";
    };


    
private:
};

}
}


#endif /* Constants_h */
