//
// Created by Brett Chaldecott on 2019/03/05.
//

#ifndef KETO_ENCRYPTIONRESPONSEPROTOHELPER_HPP
#define KETO_ENCRYPTIONRESPONSEPROTOHELPER_HPP

#include <string>
#include <memory>

#include "keto/event/Event.hpp"

#include "KeyStore.pb.h"
#include "keto/crypto/SecureVectorUtils.hpp"
#include "keto/server_common/VectorUtils.hpp"

namespace keto {
namespace key_store_utils {

class EncryptionResponseProtoHelper;
typedef std::shared_ptr<EncryptionResponseProtoHelper> EncryptionResponseProtoHelperPtr;

class EncryptionResponseProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    EncryptionResponseProtoHelper();
    EncryptionResponseProtoHelper(const std::string& value);
    EncryptionResponseProtoHelper(const keto::proto::EncryptResponse encryptResponse);
    EncryptionResponseProtoHelper(const EncryptionResponseProtoHelper& orig) = default;
    virtual ~EncryptionResponseProtoHelper();

    EncryptionResponseProtoHelper& setAsn1Message(const std::vector<uint8_t>& bytes);
    EncryptionResponseProtoHelper& operator = (const std::vector<uint8_t>& bytes);
    EncryptionResponseProtoHelper& setAsn1Message(const keto::crypto::SecureVector& bytes);
    EncryptionResponseProtoHelper& operator = (const keto::crypto::SecureVector& bytes);

    operator std::string() const;
    operator std::vector<uint8_t>() const;
    operator keto::crypto::SecureVector() const;
    operator keto::proto::EncryptResponse() const;
private:
    keto::proto::EncryptResponse encryptResponse;

};


}
}



#endif //KETO_ENCRYPTIONRESPONSEPROTOHELPER_HPP
