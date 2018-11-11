/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Exception.hpp
 * Author: ubuntu
 *
 * Created on February 14, 2018, 2:54 PM
 */

#ifndef KETO_SESSION_EXCEPTION_HPP
#define KETO_SESSION_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"

namespace keto {
namespace session {

// the keto crypto exception base
KETO_DECLARE_EXCEPTION( HttpClientSessionException, "Keto Http Client Session." );

// the 
KETO_DECLARE_DERIVED_EXCEPTION (HttpClientSessionException, ClientFailedToCloseSocketException , "The http client failed to close the connection.");
KETO_DECLARE_DERIVED_EXCEPTION (HttpClientSessionException, ClientHandShakeFailureException , "Client hand shake failure.");
KETO_DECLARE_DERIVED_EXCEPTION (HttpClientSessionException, ClientAuthorizationFailureException , "The client could not authorize with the server.");
KETO_DECLARE_DERIVED_EXCEPTION (HttpClientSessionException, UnsupportedOperationException , "This library does not support this method.");

    
}
}


#endif /* EXCEPTION_HPP */

