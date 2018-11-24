//
//  KeyStoreReEncryptTransactionMessageHelper.hpp
//  KETO
//
//  Created by Brett Chaldecott on 2018/11/22.
//

#ifndef KeyStoreReEncryptTransactionMessageHelper_h
#define KeyStoreReEncryptTransactionMessageHelper_h


// std includes
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include "keto/transaction_common/TransactionEncryptionHandler.hpp"
#include "keto/crypto/Containers.hpp"


namespace keto {
namespace keystore {

class KeyStoreReEncryptTransactionMessageHelper;
typedef std::shared_ptr<KeyStoreReEncryptTransactionMessageHelper> KeyStoreReEncryptTransactionMessageHelperPtr;

class KeyStoreReEncryptTransactionMessageHelper : public keto::transaction_common::TransactionEncryptionHandler {
public:
    KeyStoreReEncryptTransactionMessageHelper(const keto::crypto::SecureVector& privateKey);
    KeyStoreReEncryptTransactionMessageHelper(const KeyStoreReEncryptTransactionMessageHelper& orig) = delete;
    virtual~ KeyStoreReEncryptTransactionMessageHelper();
    
    
    EncryptedDataWrapper_t* encrypt(
                                    const TransactionMessage_t& transaction);
    
    TransactionMessage_t* decrypt(
                                  const EncryptedDataWrapper_t& encrypt);
    
    
private:
    keto::crypto::SecureVector privateKey;
    
    keto::crypto::SecureVector copyEncryptedToVector(const EncryptedDataWrapper_t& encrypt);
};

}
}

#endif /* KeyStoreReEncryptTransactionMessageHelper_h */
