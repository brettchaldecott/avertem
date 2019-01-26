//
// Created by Brett Chaldecott on 2019/01/23.
//

#ifndef KETO_NETWORKSESSIONMANAGER_HPP
#define KETO_NETWORKSESSIONMANAGER_HPP

#include <string>
#include <memory>
#include <vector>

#include "KeyStore.pb.h"

#include "keto/memory_vault_session/MemoryVaultSessionKeyWrapper.hpp"

#include "keto/keystore/NetworkSessionKeyEncryptor.hpp"
#include "keto/keystore/NetworkSessionKeyDecryptor.hpp"

namespace keto {
namespace keystore {

class NetworkSessionKeyManager;
typedef std::shared_ptr<NetworkSessionKeyManager> NetworkSessionKeyManagerPtr;

class NetworkSessionKeyManager {
public:
    friend class NetworkSessionKeyEncryptor;
    friend class NetworkSessionKeyDecryptor;
    inline static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    NetworkSessionKeyManager(const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator);
    NetworkSessionKeyManager(const NetworkSessionKeyManager& orig) = delete;
    virtual ~NetworkSessionKeyManager();


    static NetworkSessionKeyManagerPtr init(
            const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator);
    static NetworkSessionKeyManagerPtr getInstance();
    static void fin();

    // initialize the session key manager
    void clearSession();
    void generateSession();
    void setSession(const keto::proto::NetworkKeysWrapper& networkKeys);
    keto::proto::NetworkKeysWrapper getSession();

    // methods to get the encryptor and decryptor
    NetworkSessionKeyEncryptorPtr getEncryptor();
    NetworkSessionKeyDecryptorPtr getDecryptor();

protected:
    int getNumberOfKeys();
    keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr getKey(int index);

private:
    bool networkSessionGenerator;
    int networkSessionKeyNumber;
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator;
    std::map<std::vector<uint8_t>,keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr> sessionKeys;
    // the hash index is used to determine which key should be used to decrypt the entry
    std::vector<std::vector<uint8_t>> hashIndex;
};


}
}


#endif //KETO_NETWORKSESSIONMANAGER_HPP
