/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 14, 2018, 7:15 AM
 */

#ifndef KETO_RPC_CLIENT_EXCEPTION_HPP
#define KETO_RPC_CLIENT_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace rpc_client {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( RpcClientException, "Block exception." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (RpcClientException, PublicKeyNotConfiguredException , "The public key was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcClientException, PrivateKeyNotConfiguredException , "The private key was not found.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcClientException, ServerNotAvailableException , "The server is not available to route to.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcClientException, NoDefaultRouteAvailableException , "No default server is registered.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcClientException, ConnectionLost , "The connection to the server has been lost.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcClientException, NoActivePeerException , "There is no active peer at this point.");
    
}
}


#endif /* EXCEPTION_HPP */

