/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 21, 2018, 8:16 AM
 */

#ifndef KETO_ROUTER_DB_EXCEPTION_HPP
#define KETO_ROUTER_DB_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"

namespace keto {
namespace router_db {

// the keto db
KETO_DECLARE_EXCEPTION( RouterDBException, "Router DB Exception." );

KETO_DECLARE_DERIVED_EXCEPTION (RouterDBException, RouteDBReadException , "Failed to read values back from the database.");

    
}
}


#endif /* EXCEPTION_HPP */

