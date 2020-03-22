/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RDFModelHelper.cpp
 * Author: ubuntu
 * 
 * Created on March 12, 2018, 5:17 PM
 */

#include "ANY.h"
#include "RDFDataFormat.h"
#include "keto/asn1/RDFModelHelper.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/asn1/Exception.hpp"
#include "keto/asn1/CloneHelper.hpp"
#include "keto/asn1/StringUtils.hpp"
#include "include/keto/asn1/StringUtils.hpp"


namespace keto {
namespace asn1 {

std::string RDFModelHelper::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
RDFModelHelper::RDFModelHelper() {
    this->rdfModel = (RDFModel_t*)calloc(1, sizeof *rdfModel);
    this->rdfModel->action = RDFChange_persist;
}

RDFModelHelper::RDFModelHelper(const RDFChange_t& change) {
    this->rdfModel = (RDFModel_t*)calloc(1, sizeof *rdfModel);
    this->rdfModel->action = change;
}

RDFModelHelper::RDFModelHelper(RDFModel_t* rdfModel) : rdfModel(rdfModel) {

}

RDFModelHelper::RDFModelHelper(const AnyHelper& anyHelper) {
    this->rdfModel = anyHelper.extract<RDFModel_t>(&asn_DEF_RDFModel);
    if (!this->rdfModel) {
        BOOST_THROW_EXCEPTION(keto::asn1::InvalidAnyToTypeConversion());
    }
}
    
RDFModelHelper::RDFModelHelper(const RDFModelHelper& orig) {
    this->rdfModel = keto::asn1::clone<RDFModel>(orig.rdfModel,
            &asn_DEF_RDFModel);
}


RDFModelHelper::~RDFModelHelper() {
    if (this->rdfModel) {
        ASN_STRUCT_FREE(asn_DEF_RDFModel, this->rdfModel);
    }
}

RDFModelHelper& RDFModelHelper::setChange(const RDFChange_t& change) {
    this->rdfModel->action = change;
    return (*this);
}


RDFModelHelper& RDFModelHelper::addSubject(RDFSubjectHelper& rdfSubject) {
    //RDFDataFormat_t* dataFormat = (RDFDataFormat_t*)calloc(1, sizeof *dataFormat);
    //dataFormat->present = RDFDataFormat_PR_rdfSubject;
    //RDFSubject_t* subject = rdfSubject.operator RDFSubject_t*();
    //dataFormat->choice.rdfSubject = *subject;
    if (0!= ASN_SEQUENCE_ADD(&this->rdfModel->rdfSubjects,(RDFSubject_t*)rdfSubject)) {
        BOOST_THROW_EXCEPTION(keto::asn1::FailedToAddSubjectToModelException());
    }
    return (*this);
}

RDFModelHelper& RDFModelHelper::addGroup(RDFNtGroupHelper& rdfNtGroupHelper) {
    //RDFDataFormat_t* dataFormat = (RDFDataFormat_t*)calloc(1, sizeof *dataFormat);
    //dataFormat->present = RDFDataFormat_PR_rdfNtGroup;
    //RDFNtGroup_t* group = rdfNtGroupHelper;
    //dataFormat->choice.rdfNtGroup = *group;
    if (0!= ASN_SEQUENCE_ADD(&this->rdfModel->rdfNtGroups,(RDFNtGroup_t*)rdfNtGroupHelper)) {
        BOOST_THROW_EXCEPTION(keto::asn1::FailedToAddSubjectToModelException());
    }
    return (*this);
}

std::vector<std::string> RDFModelHelper::subjects() {
    std::vector<std::string> result;
    for (int index = 0; index < this->rdfModel->rdfSubjects.list.count; index++) {
        //if (this->rdfModel->rdfDataFormat.list.array[index]->present != RDFDataFormat_PR_rdfSubject) {
        //    continue;
        //}
        
        result.push_back(StringUtils::copyBuffer(
                this->rdfModel->rdfSubjects.list.array[index]->subject));
    }
    return result;
}

std::vector<RDFSubjectHelperPtr> RDFModelHelper::getSubjects() {
    std::vector<RDFSubjectHelperPtr> result;
    for (int index = 0; index < this->rdfModel->rdfSubjects.list.count; index++) {
        //if (this->rdfModel->rdfDataFormat.list.array[index]->present != RDFDataFormat_PR_rdfSubject) {
        //    continue;
        //}
        result.push_back(RDFSubjectHelperPtr(new RDFSubjectHelper(
                this->rdfModel->rdfSubjects.list.array[index],false)));
    }
    
    return result;
}

std::vector<RDFNtGroupHelperPtr> RDFModelHelper::getRDFNtGroups() {
    std::vector<RDFNtGroupHelperPtr> result;
    for (int index = 0; index < this->rdfModel->rdfNtGroups.list.count; index++) {
        //if (this->rdfModel->rdfDataFormat.list.array[index]->present != RDFDataFormat_PR_rdfNtGroup) {
        //    continue;
        //}
        result.push_back(RDFNtGroupHelperPtr(new RDFNtGroupHelper(
                this->rdfModel->rdfNtGroups.list.array[index],false)));
    }
    return result;
}

bool RDFModelHelper::contains(const std::string& subject) {
    for (int index = 0; index < this->rdfModel->rdfSubjects.list.count; index++) {
        //if (this->rdfModel->rdfDataFormat.list.array[index]->present != RDFDataFormat_PR_rdfSubject) {
        //    continue;
        //}
        std::string subjectName = StringUtils::copyBuffer(
                this->rdfModel->rdfSubjects.list.array[index]->subject);
        if (subjectName.compare(subject) != 0) {
            continue;
        }
        return true;
    }
    return false;
}

RDFSubjectHelperPtr RDFModelHelper::operator [](const std::string& subject) {
    for (int index = 0; index < this->rdfModel->rdfSubjects.list.count; index++) {
        //if (this->rdfModel->rdfDataFormat.list.array[index]->present != RDFDataFormat_PR_rdfSubject) {
        //    continue;
        //}
        
        std::string subjectName = StringUtils::copyBuffer(
                this->rdfModel->rdfSubjects.list.array[index]->subject);
        if (subjectName.compare(subject) != 0) {
            continue;
        }
        
        return RDFSubjectHelperPtr(new RDFSubjectHelper(
                this->rdfModel->rdfSubjects.list.array[index],false));
    }
    std::stringstream ss;
    BOOST_THROW_EXCEPTION(keto::asn1::SubjectNotFoundInModelException(ss.str()));
}

RDFModelHelper::operator RDFModel_t&() {
    return *this->rdfModel;
}

RDFModelHelper::operator RDFModel_t*() {
    RDFModel_t* result = this->rdfModel;
    this->rdfModel = 0;
    return result;
}

RDFModelHelper::operator ANY_t*() {
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_RDFModel, this->rdfModel);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
    }
    return anyPtr;
}

RDFModelHelper::operator keto::asn1::AnyHelper() const {
    ANY_t* anyPtr = ANY_new_fromType(&asn_DEF_RDFModel, this->rdfModel);
    if (!anyPtr) {
        BOOST_THROW_EXCEPTION(keto::asn1::TypeToAnyConversionFailedException());
    }
    return anyPtr;
}

}
}
