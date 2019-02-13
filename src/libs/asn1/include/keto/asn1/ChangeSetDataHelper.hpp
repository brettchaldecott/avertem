//
// Created by Brett Chaldecott on 2019/02/13.
//

#ifndef KETO_CHANGESETDATAHELPER_HPP
#define KETO_CHANGESETDATAHELPER_HPP

#include <string>
#include <memory>

#include "Status.h"
#include "ChangeData.h"
#include "ChangeSet.h"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/AnyHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"
#include "keto/asn1/ChangeSetDataHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace asn1 {

class ChangeSetDataHelper;
typedef std::shared_ptr<ChangeSetDataHelper> ChangeSetDataHelperPtr;

class ChangeSetDataHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ChangeSetDataHelper(ChangeData_t* changeData);
    ChangeSetDataHelper(const ChangeSetDataHelper& orig) = delete;
    virtual ~ChangeSetDataHelper();

    bool isASN1();
    keto::asn1::AnyHelper getAny();
    std::vector<uint8_t> getBytes();

private:
    ChangeData_t* changeData;
};


}
}



#endif //KETO_CHANGESETDATAHELPER_HPP
