//
// Created by Brett Chaldecott on 2019/03/05.
//

#ifndef KETO_ENCRYPTIONREQUESTPROTOHELPER_HPP
#define KETO_ENCRYPTIONREQUESTPROTOHELPER_HPP


#include <string>
#include <memory>

#include "keto/event/Event.hpp"

#include "KeyStore.pb.h"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {

class EncryptionRequestProtoHelper;
typedef std::shared_ptr<EncryptionRequestProtoHelper> EncryptionRequestProtoHelperPtr;

class EncryptionRequestProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    EncryptionRequestProtoHelper();
    EncryptionRequestProtoHelper(const std::string& value);
    EncryptionRequestProtoHelper(const keto::proto::EncryptRequest encryptRequest);
    EncryptionRequestProtoHelper(const EncryptionRequestProtoHelper& orig) = default;
    virtual ~EncryptionRequestProtoHelper();

    EncryptionRequestProtoHelper& setAsn1Message(const std::vector<uint8_t>& bytes);
    EncryptionRequestProtoHelper& operator = (const std::vector<uint8_t>& bytes);
    EncryptionRequestProtoHelper& setAsn1Message(const keto::crypto::SecureVector& bytes);
    EncryptionRequestProtoHelper& operator = (const keto::crypto::SecureVector& bytes);

    operator std::vector<uint8_t>() const;
    operator keto::crypto::SecureVector() const;
    operator keto::proto::EncryptRequest() const;
private:
    keto::proto::EncryptRequest encryptRequest;

};


}
}

#endif //KETO_ENCRYPTIONREQUESTPROTOHELPER_HPP
