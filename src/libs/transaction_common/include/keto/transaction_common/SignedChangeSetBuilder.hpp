/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignedChangeSetBuilder.hpp
 * Author: ubuntu
 *
 * Created on March 17, 2018, 5:30 AM
 */

#ifndef TRANSACTION_COMMON_SIGNEDCHANGESETBUILDER_HPP
#define TRANSACTION_COMMON_SIGNEDCHANGESETBUILDER_HPP

#include <string>
#include <memory>

#include "ChangeSet.h"
#include "SignedChangeSet.h"

#include "keto/crypto/KeyLoader.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace transaction_common {

class SignedChangeSetBuilder;
typedef std::shared_ptr<SignedChangeSetBuilder> SignedChangeSetBuilderPtr;


class SignedChangeSetBuilder {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    SignedChangeSetBuilder();
    SignedChangeSetBuilder(ChangeSet_t* changeSet);
    SignedChangeSetBuilder(ChangeSet_t* changeSet, const keto::crypto::KeyLoader& keyloader);
    SignedChangeSetBuilder(const SignedChangeSetBuilder& orig) = delete;
    virtual ~SignedChangeSetBuilder();
    
    
    SignedChangeSetBuilder& setChangeSet(ChangeSet_t* changeSet);
    SignedChangeSetBuilder& setKeyLoader(const keto::crypto::KeyLoader& keyloader);
    SignedChangeSetBuilder& sign();
    
    operator SignedChangeSet_t*();
    operator SignedChangeSet_t&();
    
private:
    SignedChangeSet_t* signedChangedSet;
    keto::crypto::KeyLoader keyLoader;
};


}
}

#endif /* SIGNEDCHANGESETBUILDER_HPP */

