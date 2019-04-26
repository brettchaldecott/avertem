//
// Created by Brett Chaldecott on 2019/03/05.
//

#ifndef KETO_ENCRYPTION_NETWORK_RESPONSEPROTOHELPER_HPP
#define KETO_ENCRYPTION_NETWORK_RESPONSEPROTOHELPER_HPP

#include <string>
#include <memory>

#include "keto/event/Event.hpp"

#include "KeyStore.pb.h"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {

class EncryptionNetworkResponseProtoHelper;
typedef std::shared_ptr<EncryptionNetworkResponseProtoHelper> EncryptionNetworkResponseProtoHelperPtr;

class EncryptionNetworkResponseProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    EncryptionNetworkResponseProtoHelper();
    EncryptionNetworkResponseProtoHelper(const std::string& value);
    EncryptionNetworkResponseProtoHelper(const keto::proto::EncryptNetworkResponse& encryptNetworkResponse);
    EncryptionNetworkResponseProtoHelper(const EncryptionNetworkResponseProtoHelper& orig) = default;
    virtual ~EncryptionNetworkResponseProtoHelper();

    EncryptionNetworkResponseProtoHelper& setBytesMessage(const std::vector<uint8_t>& bytes);
    EncryptionNetworkResponseProtoHelper& operator = (const std::vector<uint8_t>& bytes);
    EncryptionNetworkResponseProtoHelper& setBytesMessage(const keto::crypto::SecureVector& bytes);
    EncryptionNetworkResponseProtoHelper& operator = (const keto::crypto::SecureVector& bytes);

    operator std::string() const;
    operator std::vector<uint8_t>() const;
    operator keto::crypto::SecureVector() const;
    operator keto::proto::EncryptNetworkResponse() const;
private:
    keto::proto::EncryptNetworkResponse encryptNetworkResponse;

};


}
}



#endif //KETO_ENCRYPTIONRESPONSEPROTOHELPER_HPP
