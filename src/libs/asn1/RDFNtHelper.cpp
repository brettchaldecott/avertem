//
// Created by Brett Chaldecott on 2020/02/28.
//

#include "keto/asn1/RDFNtHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"
#include "keto/asn1/StringUtils.hpp"

namespace keto {
namespace asn1 {


std::string RDFNtHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RDFNtHelper::RDFNtHelper() : own(true) {
    this->rdfNt = (RDFNT_t*)calloc(1, sizeof *rdfNt);
    this->rdfNt->version = 1;
}

RDFNtHelper::RDFNtHelper(RDFNT_t* rdfNt) : rdfNt(rdfNt), own(false) {
}

RDFNtHelper::RDFNtHelper(RDFNT_t* rdfNt, bool own) : rdfNt(rdfNt), own(own) {
}

RDFNtHelper::RDFNtHelper(const std::string &subject, const std::string &predicate, const std::string &object) {
    this->rdfNt = (RDFNT_t*)calloc(1, sizeof *rdfNt);
    OCTET_STRING_fromBuf(&rdfNt->ntSubject,
                         subject.c_str(),subject.size());
    OCTET_STRING_fromBuf(&rdfNt->ntPredicate,
                         predicate.c_str(),predicate.size());
    OCTET_STRING_fromBuf(&rdfNt->ntObject,
                         object.c_str(),object.size());
}

RDFNtHelper::~RDFNtHelper() {
    if (own) {
        ASN_STRUCT_FREE(asn_DEF_RDFNT, this->rdfNt);
        this->rdfNt = 0;
    }
}


RDFNtHelper::operator RDFNT_t *() const {
    return keto::asn1::clone<RDFNT_t>(this->rdfNt, &asn_DEF_RDFNT);
}

RDFNtHelper::operator ANY_t *() const {
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_RDFNT, this->rdfNt);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
    }
    return anyPtr;
}


RDFNtHelper& RDFNtHelper::setSubject(const std::string &value) {
    if (!own) {
        BOOST_THROW_EXCEPTION(keto::asn1::InvalidOperationObject());
    }
    OCTET_STRING_fromBuf(&rdfNt->ntSubject,
                         value.c_str(),value.size());
    return *this;
}

std::string RDFNtHelper::getSubject() {
    return StringUtils::copyBuffer(this->rdfNt->ntSubject);
}

RDFNtHelper& RDFNtHelper::setPredicate(const std::string &value) {
    if (!own) {
        BOOST_THROW_EXCEPTION(keto::asn1::InvalidOperationObject());
    }
    OCTET_STRING_fromBuf(&rdfNt->ntPredicate,
                         value.c_str(),value.size());
    return *this;
}

std::string RDFNtHelper::getPredicate() {
    return StringUtils::copyBuffer(this->rdfNt->ntPredicate);
}

RDFNtHelper& RDFNtHelper::setObject(const std::string &value) {
    if (!own) {
        BOOST_THROW_EXCEPTION(keto::asn1::InvalidOperationObject());
    }
    OCTET_STRING_fromBuf(&rdfNt->ntObject,
                         value.c_str(),value.size());
    return *this;
}

std::string RDFNtHelper::getObject() {
    return StringUtils::copyBuffer(this->rdfNt->ntObject);
}

}
}