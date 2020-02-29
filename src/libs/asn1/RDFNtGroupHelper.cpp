//
// Created by Brett Chaldecott on 2020/02/28.
//

#include "keto/asn1/RDFNtGroupHelper.hpp"
#include "keto/asn1/CloneHelper.hpp"
#include "keto/asn1/StringUtils.hpp"


namespace keto {
namespace asn1 {

std::string RDFNtGroupHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RDFNtGroupHelper::RDFNtGroupHelper() : own(true) {
    this->rdfNtGroup = (RDFNtGroup_t*)calloc(1, sizeof *rdfNtGroup);
    this->rdfNtGroup->version = 1;
}

RDFNtGroupHelper::RDFNtGroupHelper(RDFNtGroup_t* rdfNtGroup) : rdfNtGroup(rdfNtGroup), own(false) {
}


RDFNtGroupHelper::RDFNtGroupHelper(RDFNtGroup_t* rdfNtGroup, bool own) : rdfNtGroup(rdfNtGroup), own(own) {

}

RDFNtGroupHelper::~RDFNtGroupHelper() {
    if (own) {
        ASN_STRUCT_FREE(asn_DEF_RDFNtGroup, this->rdfNtGroup);
        this->rdfNtGroup = 0;
    }
}

RDFNtGroupHelper::operator RDFNtGroup_t*() {
    return keto::asn1::clone<RDFNtGroup_t>(this->rdfNtGroup, &asn_DEF_RDFNtGroup);
}

RDFNtGroupHelper::operator ANY_t*(){
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_RDFNtGroup, this->rdfNtGroup);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
    }
    return anyPtr;
}

RDFNtGroupHelper& RDFNtGroupHelper::addRDFNT(const RDFNtHelper& rdfNtHelper)  {
    if (!own) {
        BOOST_THROW_EXCEPTION(keto::asn1::InvalidOperationObject());
    }
    if (0!= ASN_SEQUENCE_ADD(&this->rdfNtGroup->rdfNT,(RDFNT_t*)rdfNtHelper)) {
        BOOST_THROW_EXCEPTION(keto::asn1::FailedToAddNTtoGropudException());
    }
    return *this;
}

RDFNtGroupHelper& RDFNtGroupHelper::addRDFNT(const RDFNtHelperPtr& rdfNtHelper) {
    if (!own) {
        BOOST_THROW_EXCEPTION(keto::asn1::InvalidOperationObject());
    }
    if (0!= ASN_SEQUENCE_ADD(&this->rdfNtGroup->rdfNT,(RDFNT_t*)*rdfNtHelper)) {
        BOOST_THROW_EXCEPTION(keto::asn1::FailedToAddNTtoGropudException());
    }
    return *this;
}

std::vector<RDFNtHelperPtr> RDFNtGroupHelper::getRDFNts() {
    std::vector<RDFNtHelperPtr> result;
    for (int index = 0; index < this->rdfNtGroup->rdfNT.list.count; index++) {
        result.push_back(RDFNtHelperPtr(new RDFNtHelper(
                this->rdfNtGroup->rdfNT.list.array[index],false)));
    }

    return result;
}

}
}