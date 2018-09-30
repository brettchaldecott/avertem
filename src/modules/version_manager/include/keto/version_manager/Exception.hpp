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

#ifndef KETO_BLOCK_EXCEPTION_HPP
#define KETO_BLOCK_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace version_manager {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( VersionManagerException, "Version Manager Exception." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (VersionManagerException, AutoUpdateNotConfiguredException , "The auto update is not configured.");
KETO_DECLARE_DERIVED_EXCEPTION (VersionManagerException, CheckScriptNotConfiguredException , "The auto update is not configured.");

    
}
}


#endif /* EXCEPTION_HPP */

