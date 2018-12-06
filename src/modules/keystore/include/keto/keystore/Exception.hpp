//
//  Excepition.hpp
//  KETO
//
//  Created by Brett Chaldecott on 2018/11/21.
//

#ifndef KETO_KEY_STORE_EXCEPTION
#define KETO_KEY_STORE_EXCEPTION


#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace keystore {
    
// the keto module exception base
KETO_DECLARE_EXCEPTION( KeyStoreModuleException, "General key store exception." );


// the keto module derived exception
KETO_DECLARE_DERIVED_EXCEPTION (KeyStoreModuleException, InvalidSessionKeyException , "Invalid session key.");
KETO_DECLARE_DERIVED_EXCEPTION (KeyStoreModuleException, PrivateKeyNotConfiguredException , "Private key has not been configured.");
KETO_DECLARE_DERIVED_EXCEPTION (KeyStoreModuleException, PublicKeyNotConfiguredException , "Public key has not been configured.");



}
}



#endif /* Excepition_h */
