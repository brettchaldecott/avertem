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

#ifndef KETO_ROUTER_MODULE_EXCEPTION_HPP
#define KETO_ROUTER_MODULE_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace router {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( RouterModuleException, "Keto Router Module Exception." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (RouterModuleException, NoServicesRegisteredException , "No services registered.");
KETO_DECLARE_DERIVED_EXCEPTION (RouterModuleException, NoAccountsForServiceException , "No accounts for the service.");
KETO_DECLARE_DERIVED_EXCEPTION (RouterModuleException, NoPeersRegistered , "No Registered Peers.");
KETO_DECLARE_DERIVED_EXCEPTION (RouterModuleException, NoGrowingTangle , "No growing tangle.");

}
}

#endif /* EXCEPTION_HPP */

