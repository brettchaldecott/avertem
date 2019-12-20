/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 31, 2018, 4:37 PM
 */

#ifndef KETO_SANDBOX_MODULE_EXCEPTION_HPP
#define KETO_SANDBOX_MODULE_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace sandbox {

// the keto environment exception base
KETO_DECLARE_EXCEPTION( WavmCommonException, "Wavm common failed." );

// the keto wavm derived exception
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ChildRequestException , "Parent requested resulted in an exception.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ChildExitedUnexpectedly , "Child exited unexpectedly.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, CloneFailedException , "Clone failed exception.");
KETO_DECLARE_DERIVED_EXCEPTION (WavmCommonException, ChildStackCreationFailedException , "Child stack creation failed exception.");


}
}

#endif /* EXCEPTION_HPP */

