/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountSystemOntologyTypes.hpp
 * Author: ubuntu
 *
 * Created on March 26, 2018, 2:59 AM
 */

#ifndef ACCOUNTSYSTEMONTOLOGYTYPES_HPP
#define ACCOUNTSYSTEMONTOLOGYTYPES_HPP

#include <string>
#include <vector>

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace account_db {


class AccountSystemOntologyTypes {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    static const char* CHAIN_ONTOLOGY_CLASS;

    // CHAIN PREDICATE identifiers
    class CHAIN_PREDICATES {
    public:
        static const char* ID;
    };

    static const char* BLOCK_ONTOLOGY_CLASS;

    // BLOCK PREDICATE identifiers
    class BLOCK_PREDICATES {
    public:
        static const char* ID;
        static const char* CHAIN;
    };

    static const char* TRANSACTION_ONTOLOGY_CLASS;

    // BLOCK PREDICATE identifiers
    class TRANSACTION_PREDICATES {
    public:
        static const char* ID;
        static const char* BLOCK;
        static const char* DATE;
        static const char* ACCOUNT;
    };

    static const char* ACCOUNT_DIRTY_ONTOLOGY_URI;

    static const char* ACCOUNT_ONTOLOGY_CLASS;
    static const char* GROUP_ONTOLOGY_CLASS;
    
    // PREDICATE identifiers
    class ACCOUNT_PREDICATES {
    public:
        static const char* STATUS;
        static const char* ID;
        static const char* PARENT;
        static const char* TYPE;
        static const char* TRANSACTION;
    };
    
    class ACCOUNT_TYPE {
    public:
        static const char* MASTER;
        static const char* ROOT;
        static const char* SYSTEM;
        static const char* PROXY;
        static const char* STANDARD;
        static const char* SLAVE;
        
    };

    // PREDICATE identifiers
    class ACCOUNT_PERMISSIONS {
    public:
        static const char* OWNER;
        static const char* GROUP;
    };
    
    // object literal values
    static const char* ACCOUNT_CREATE_OBJECT_STATUS;
    
    
    static const std::vector<std::string> ONTOLOGY_CLASSES; 
    
    static bool validateClassOperation(
        const keto::asn1::HashHelper& accountHash,
        const bool existingAccount,
        const keto::asn1::RDFSubjectHelperPtr& rdfSubjectHelperPtr);
    static bool isAccountOntologyClass(const keto::asn1::RDFSubjectHelperPtr& rdfSubjectHelperPtr);
private:
    
    
};


}
}
#endif /* ACCOUNTSYSTEMONTOLOGYTYPES_HPP */

