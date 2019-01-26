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
namespace rpc_protocol {
    
// the keto module exception base
KETO_DECLARE_EXCEPTION( RpcProtocolException, "General protocol exceptions." );


// the keto module derived exception
KETO_DECLARE_DERIVED_EXCEPTION (RpcProtocolException, NetworkKeysWrapperDeserializationErrorException , "Failed to deserialize the network key.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcProtocolException, NetworkKeysDeserializationErrorException , "Failed to deserialize the network key.");


}
}



#endif /* Excepition_h */
