//
// Created by Brett Chaldecott on 2020/02/28.
//

#ifndef KETO_RDFNTGROUPHELPER_HPP
#define KETO_RDFNTGROUPHELPER_HPP

#include <string>
#include <vector>
#include <memory>


#include "keto/asn1/AnyHelper.hpp"
#include "keto/asn1/RDFNtHelper.hpp"
#include "RDFNtGroup.h"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace asn1 {

class RDFNtGroupHelper;
typedef std::shared_ptr<RDFNtGroupHelper> RDFNtGroupHelperPtr;

class RDFNtGroupHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RDFNtGroupHelper();
    RDFNtGroupHelper(RDFNtGroup_t* rdfNtGroup);
    RDFNtGroupHelper(RDFNtGroup_t* rdfNtGroup, bool own);
    RDFNtGroupHelper(const RDFNtGroupHelper& orig) = delete;
    virtual ~RDFNtGroupHelper();

    operator RDFNtGroup_t*();
    operator ANY_t*();

    RDFNtGroupHelper& addRDFNT(const RDFNtHelper& rdfNtHelper);
    RDFNtGroupHelper& addRDFNT(const RDFNtHelperPtr& rdfNtHelper);
    std::vector<RDFNtHelperPtr> getRDFNts();

private:
    RDFNtGroup_t* rdfNtGroup;
    bool own;
};

}
}


#endif //KETO_RDFNTGROUPHELPER_HPP
