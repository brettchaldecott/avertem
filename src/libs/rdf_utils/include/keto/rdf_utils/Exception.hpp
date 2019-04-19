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

#ifndef KETO_RDF_UTILS_EXCEPTION_HPP
#define KETO_RDF_UTILS_EXCEPTION_HPP

#include <string>
#include "keto/common/Exception.hpp"

namespace keto {
namespace rdf_utils {

// the keto db
KETO_DECLARE_EXCEPTION( RDFUtilsException, "RDF Utils Exception." );

KETO_DECLARE_DERIVED_EXCEPTION (RDFUtilsException, InvalidQueryPatternException , "Invalid query missing a pattern.");
KETO_DECLARE_DERIVED_EXCEPTION (RDFUtilsException, InvalidSubjectVariablePatternException , "Invalid subject variable pattern.");
KETO_DECLARE_DERIVED_EXCEPTION (RDFUtilsException, FailedToProcessQueryException , "Failed to process the query..");
    
}
}


#endif /* EXCEPTION_HPP */

