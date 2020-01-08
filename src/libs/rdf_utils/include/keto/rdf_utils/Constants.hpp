/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on February 13, 2018, 7:28 AM
 */

#ifndef KETO_CLI_CONSTANTS_HPP
#define KETO_CLI_CONSTANTS_HPP

#include <vector>

namespace keto {
namespace rdf_utils {


class Constants {
public:
    static constexpr const char* SPARQL = "sparql";

    static constexpr const char* ACCOUNT_OWNER_REF = "http://keto-coin.io/schema/rdf/1.0/keto/Account#accountOwner";
    static constexpr const char* ACCOUNT_OWNER_URI = "http://keto-coin.io/schema/rdf/1.0/keto/Account#Account/";
    static constexpr const char* ACCOUNT_OWNER_SUBJECT = "__accountOwnerSubject";
    static constexpr const char* ACCOUNT_GROUP_REF = "http://keto-coin.io/schema/rdf/1.0/keto/AccountGroup#accountGroup";
    static constexpr const char* ACCOUNT_GROUP_URI = "http://keto-coin.io/schema/rdf/1.0/keto/AccountGroup#AccountGroup/";
    static constexpr const char* ACCOUNT_GROUP_SUBJECT = "__accountGroupSubject";

    static constexpr const int  SPARQL_DEFAULT_LIMIT = 250;
    static constexpr const int  SPARQL_MAX_LIMIT = 2500;

    class EXCLUDE {
    public:
        static constexpr const char* TRANSACTION = "http://keto-coin.io/schema/rdf/1.0/keto/Transaction#";
        static constexpr const char* BLOCK = "http://keto-coin.io/schema/rdf/1.0/keto/Block#";
    };

    static const std::vector<const char*> EXCLUDES;

};



}
}

#endif /* CONSTANTS_HPP */

