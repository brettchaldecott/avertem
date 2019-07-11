/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 16, 2018, 8:59 AM
 */

#ifndef KETO_RPC_SERVER_MODULE_EXCEPTION_HPP
#define KETO_RPC_SERVER_MODULE_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace rpc_server {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( RpcServerModuleException, "Keto Rpc Server Module Exception." );

// the client is not available
KETO_DECLARE_DERIVED_EXCEPTION (RpcServerModuleException, ClientNotAvailableException , "The client is not available.");
KETO_DECLARE_DERIVED_EXCEPTION (RpcServerModuleException, ConnectionLost , "The connection to the client has been lost.");

}
}

#endif /* EXCEPTION_HPP */

