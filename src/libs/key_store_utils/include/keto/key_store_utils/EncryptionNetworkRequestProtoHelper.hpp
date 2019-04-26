//
// Created by Brett Chaldecott on 2019/03/05.
//

#ifndef KETO_ENCRYPTION_NETWORK_REQUESTPROTOHELPER_HPP
#define KETO_ENCRYPTION_NETWORK_REQUESTPROTOHELPER_HPP


#include <string>
#include <memory>

#include "keto/event/Event.hpp"

#include "KeyStore.pb.h"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {

class EncryptionNetworkRequestProtoHelper;
typedef std::shared_ptr<EncryptionNetworkRequestProtoHelper> EncryptionNetworkRequestProtoHelperPtr;

class EncryptionNetworkRequestProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    EncryptionNetworkRequestProtoHelper();
    EncryptionNetworkRequestProtoHelper(const std::string& value);
    EncryptionNetworkRequestProtoHelper(const std::vector<uint8_t>& bytes);
    EncryptionNetworkRequestProtoHelper(const keto::proto::EncryptNetworkRequest& encryptNetworkRequest);
    EncryptionNetworkRequestProtoHelper(const EncryptionNetworkRequestProtoHelper& orig) = default;
    virtual ~EncryptionNetworkRequestProtoHelper();

    EncryptionNetworkRequestProtoHelper& setByteMessage(const std::vector<uint8_t>& bytes);
    EncryptionNetworkRequestProtoHelper& operator = (const std::vector<uint8_t>& bytes);
    EncryptionNetworkRequestProtoHelper& setByteMessage(const keto::crypto::SecureVector& bytes);
    EncryptionNetworkRequestProtoHelper& operator = (const keto::crypto::SecureVector& bytes);

    operator std::vector<uint8_t>() const;
    operator keto::crypto::SecureVector() const;
    operator keto::proto::EncryptNetworkRequest() const;
private:
    keto::proto::EncryptNetworkRequest encryptNetworkRequest;

};


}
}

#endif //KETO_ENCRYPTIONREQUESTPROTOHELPER_HPP
