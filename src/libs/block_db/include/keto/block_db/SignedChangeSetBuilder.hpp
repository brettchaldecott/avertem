/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignedChangeSetBuilder.hpp
 * Author: ubuntu
 *
 * Created on March 13, 2018, 1:56 PM
 */

#ifndef SIGNEDCHANGESETBUILDER_HPP
#define SIGNEDCHANGESETBUILDER_HPP

#include <string>
#include <memory>

#include "SignedChangeSet.h"

#include "keto/asn1/PrivateKeyHelper.hpp"
#include "keto/asn1/ChangeSetHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {
    
class SignedChangeSetBuilder;
typedef std::shared_ptr<SignedChangeSetBuilder> SignedChangeSetBuilderPtr;

class SignedChangeSetBuilder {
public:
    static std::string getVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();
    
    SignedChangeSetBuilder();
    SignedChangeSetBuilder(
            const keto::asn1::PrivateKeyHelper& privateKeyHelper,
            keto::asn1::ChangeSetHelper& changeSetHelper);
    SignedChangeSetBuilder(const SignedChangeSetBuilder& orig) = delete;
    virtual ~SignedChangeSetBuilder();
    
    SignedChangeSetBuilder& setPrivateKey(
            const keto::asn1::PrivateKeyHelper& privateKeyHelper);
    SignedChangeSetBuilder& setChangeSet(
            keto::asn1::ChangeSetHelper& changeSetHelper);
    SignedChangeSetBuilder& sign();
    operator SignedChangeSet_t*();
    
    keto::asn1::HashHelper getHash();
    
private:
    keto::asn1::PrivateKeyHelper privateKeyHelper;
    SignedChangeSet_t* signedChangeSet;

    keto::asn1::HashHelper getChangeSetHash(ChangeSet_t* changeSet);
    

};



}
}

#endif /* SIGNEDCHANGESETBUILDER_HPP */

