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
    
private:
};

}
}


#endif /* Constants_h */
