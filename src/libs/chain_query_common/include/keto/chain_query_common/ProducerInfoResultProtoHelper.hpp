//
// Created by Brett Chaldecott on 2019-09-24.
//

#ifndef KETO_PRODUCERINFORESULTPROTOHELPER_HPP
#define KETO_PRODUCERINFORESULTPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

namespace keto {
namespace chain_query_common {

class ProducerInfoResultProtoHelper;
typedef std::shared_ptr<ProducerInfoResultProtoHelper> ProducerInfoResultProtoHelperPtr;

class ProducerInfoResultProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ProducerInfoResultProtoHelper();
    ProducerInfoResultProtoHelper(const keto::proto::ProducerInfoResult& producerInfoResult);
    ProducerInfoResultProtoHelper(const std::string& msg);
    ProducerInfoResultProtoHelper(const ProducerInfoResultProtoHelper& orig) = default;
    virtual ~ProducerInfoResultProtoHelper();

    keto::asn1::HashHelper getAccountHashId();
    ProducerInfoResultProtoHelper& setAccountHashId(const keto::asn1::HashHelper& hashHelper);

    std::vector<keto::asn1::HashHelper> getTangles();
    ProducerInfoResultProtoHelper& setTangles(const std::vector<keto::asn1::HashHelper>& tangles);
    ProducerInfoResultProtoHelper& addTangle(const keto::asn1::HashHelper& tangle);

    operator keto::proto::ProducerInfoResult() const;
    operator std::string() const;

private:
    keto::proto::ProducerInfoResult producerInfoResult;

};

}
}


#endif //KETO_PRODUCERINFORESULTPROTOHELPER_HPP
