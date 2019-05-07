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

#ifndef KETO_KEY_TOOLS_EXCEPTION_HPP
#define KETO_KEY_TOOLS_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"


namespace keto {
namespace key_tools {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( KeyToolsException, "Key Tools Exeption." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (KeyToolsException, InvalidKeyDataException , "The key data supplied is invalid.");

}
}

#endif /* EXCEPTION_HPP */

