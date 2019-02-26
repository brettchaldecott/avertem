/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AnyHelper.cpp
 * Author: ubuntu
 * 
 * Created on February 3, 2018, 11:15 AM
 */

#include "keto/asn1/AnyHelper.hpp"
#include "keto/asn1/AnyInterface.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/DeserializationHelper.hpp"
#include <stdlib.h>

namespace keto {
namespace asn1 {

std::string AnyHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
AnyHelper::AnyHelper(AnyInterface* anyInterface) : 
    anyInterface(anyInterface)  {
    this->any = 0;
    
}

AnyHelper::AnyHelper(ANY_t& any) {
    this->any = keto::asn1::DeserializationHelper<ANY_t>(
        keto::asn1::SerializationHelper<ANY_t>(&any,&asn_DEF_ANY).
        operator std::vector<uint8_t>&(),&asn_DEF_ANY).takePtr();
    
    this->anyInterface = 0;
}

AnyHelper::AnyHelper(ANY_t* any) : 
    any(any)  {
    this->anyInterface = 0;
}

AnyHelper::AnyHelper(const std::string& value) {
    this->anyInterface = 0;
    std::vector<uint8_t> bytes;
    for (uint8_t entry: value) {
        bytes.push_back(entry);
    }
    this->any = keto::asn1::DeserializationHelper<ANY_t>(bytes,&asn_DEF_ANY).takePtr();
}

AnyHelper::~AnyHelper() {
    if (any) {
        //free(this->any->buf);
        //free(this->any);
        ASN_STRUCT_FREE(asn_DEF_ANY, this->any);
        any=0;
    }
}

AnyHelper::operator ANY_t() {
    ANY_t result;
    if (any) {
        // duplicate the any
        ANY_t* ptr = keto::asn1::DeserializationHelper<ANY_t>(
            keto::asn1::SerializationHelper<ANY_t>(this->any,&asn_DEF_ANY).
            operator std::vector<uint8_t>&(),&asn_DEF_ANY).takePtr();
        result = *ptr;
        free(ptr);
    } else {
        if (!anyInterface) {
            BOOST_THROW_EXCEPTION(keto::asn1::NoAnyTypeInfoFailedException());
        }
        ANY_t* ptr = ANY_new_fromType(anyInterface->getType(), anyInterface->getPtr());
        if (!ptr) {
            BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
        }
        result = (*ptr);
        free(ptr);
    }
    return result;
}

AnyHelper::operator std::string() const {
    std::vector<uint8_t> bytes;
    if (any) {
        // duplicate the any
        bytes = keto::asn1::SerializationHelper<ANY_t>(this->any,&asn_DEF_ANY);
    } else {
        if (!anyInterface) {
            BOOST_THROW_EXCEPTION(keto::asn1::NoAnyTypeInfoFailedException());
        }
        ANY_t* ptr = ANY_new_fromType(anyInterface->getType(), anyInterface->getPtr());
        if (!ptr) {
            BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
        }

        bytes = keto::asn1::SerializationHelper<ANY_t>(ptr,&asn_DEF_ANY);
        ASN_STRUCT_FREE(asn_DEF_ANY, ptr);
    }
    std::string result;
    for (uint8_t entry: bytes) {
        result += entry;
    }
    return result;
}

}
}