//
// Created by Brett Chaldecott on 2020/02/28.
//

#ifndef KETO_RDFNTHELPER_HPP
#define KETO_RDFNTHELPER_HPP

#include <string>
#include <vector>
#include <memory>

#include "RDFNT.h"

#include "keto/asn1/AnyHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace asn1 {

class RDFNtHelper;
typedef std::shared_ptr<RDFNtHelper> RDFNtHelperPtr;

class RDFNtHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RDFNtHelper();
    RDFNtHelper(RDFNT_t* rdfNt);
    RDFNtHelper(RDFNT_t* rdfNt, bool own);
    RDFNtHelper(const std::string &subject, const std::string &predicate, const std::string &object);
    RDFNtHelper(const RDFNtHelper& orig) = delete;
    virtual ~RDFNtHelper();


    operator RDFNT_t*() const;
    operator ANY_t*() const;


    RDFNtHelper& setSubject(const std::string& value);
    std::string getSubject();

    RDFNtHelper& setPredicate(const std::string& value);
    std::string getPredicate();

    RDFNtHelper& setObject(const std::string& value);
    std::string getObject();


private:
    RDFNT_t* rdfNt;
    bool own;


};

}
}

#endif //KETO_RDFNTHELPER_HPP
