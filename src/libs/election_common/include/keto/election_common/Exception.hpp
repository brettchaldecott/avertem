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

#ifndef KETO_ELECTION_COMMON_EXCEPTION_HPP
#define KETO_ELECTION_COMMON_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace election_common {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( ElectionCommonException, "Keto Election Common Exception." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (ElectionCommonException, FailedToAddAlternativeException , "Failed to add an alternative to the list of accounts.");

}
}

#endif /* EXCEPTION_HPP */
