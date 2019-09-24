//
// Created by Brett Chaldecott on 2019-09-24.
//

#ifndef KETO_PRODUCERRESULTPROTOHELPER_HPP
#define KETO_PRODUCERRESULTPROTOHELPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/asn1/HashHelper.hpp"
#include "keto/asn1/SignatureHelper.hpp"

#include "keto/chain_query_common/ProducerInfoResultProtoHelper.hpp"

namespace keto {
namespace chain_query_common {

class ProducerResultProtoHelper;
typedef std::shared_ptr<ProducerResultProtoHelper> ProducerResultProtoHelperPtr;

class ProducerResultProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ProducerResultProtoHelper();
    ProducerResultProtoHelper(const keto::proto::ProducerResult& producerResult);
    ProducerResultProtoHelper(const std::string& msg);
    ProducerResultProtoHelper(const ProducerResultProtoHelper& orig) = default;
    virtual ~ProducerResultProtoHelper();

    std::vector<ProducerInfoResultProtoHelperPtr> getProducers();
    ProducerResultProtoHelper& addProducer(const ProducerInfoResultProtoHelper& producer);
    ProducerResultProtoHelper& addProducer(const ProducerInfoResultProtoHelperPtr& producer);

    operator keto::proto::ProducerResult() const;
    operator std::string() const;

private:
    keto::proto::ProducerResult producerResult;
};


}
}


#endif //KETO_PRODUCERRESULTPROTOHELPER_HPP
