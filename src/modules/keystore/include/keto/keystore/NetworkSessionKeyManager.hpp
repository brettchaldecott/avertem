//
// Created by Brett Chaldecott on 2019/01/23.
//

#ifndef KETO_NETWORKSESSIONMANAGER_HPP
#define KETO_NETWORKSESSIONMANAGER_HPP

#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "KeyStore.pb.h"

#include "keto/rpc_protocol/NetworkKeysWrapperHelper.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"
#include "keto/rpc_protocol/NetworkKeyHelper.hpp"


#include "keto/memory_vault_session/MemoryVaultSessionKeyWrapper.hpp"

#include "keto/event/Event.hpp"
#include "keto/keystore/NetworkSessionKeyEncryptor.hpp"
#include "keto/keystore/NetworkSessionKeyDecryptor.hpp"

namespace keto {
namespace keystore {

class NetworkSessionKeyManager;
typedef std::shared_ptr<NetworkSessionKeyManager> NetworkSessionKeyManagerPtr;
typedef std::map<std::vector<uint8_t>,keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr> SessionMap;

class NetworkSessionKeyManager {
public:
    class NetworkSessionSlot {
    public:
        NetworkSessionSlot(uint8_t slot, std::vector<std::vector<uint8_t>> hashIndex, const SessionMap& sessionKeys);
        NetworkSessionSlot(const NetworkSessionSlot& orig) = delete;
        virtual ~NetworkSessionSlot();

        int getNumberOfKeys();
        keto::proto::NetworkKeysWrapper getSession();

        keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr getKey(int index);
        uint8_t getSlot();
        bool checkHashIndex(const std::vector<keto::rpc_protocol::NetworkKeyHelper>& networkKeyHelpers);
    private:
        uint8_t slot;
        SessionMap sessionKeys;
        // the hash index is used to determine which key should be used to decrypt the entry
        std::vector<std::vector<uint8_t>> hashIndex;
    };
    typedef std::shared_ptr<NetworkSessionSlot> NetworkSessionSlotPtr;

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

    // network session key variables
    keto::event::Event getNetworkSessionKeys(const keto::event::Event& event);
    keto::event::Event setNetworkSessionKeys(const keto::event::Event& event);

    // methods to get the encryptor and decryptor
    NetworkSessionKeyEncryptorPtr getEncryptor();
    NetworkSessionKeyDecryptorPtr getDecryptor();

protected:
    int getNumberOfKeys();
    int getSlot();
    keto::memory_vault_session::MemoryVaultSessionKeyWrapperPtr getKey(int slot, int index);

private:
    std::mutex classMutex;
    bool networkSessionGenerator;
    bool networkSessionConfigured;
    int networkSessionKeyNumber;

    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator;
    std::map<int,NetworkSessionSlotPtr> sessionSlots;
    uint8_t slot;
    std::deque<int> slots;


    void popSlot();
};


}
}


#endif //KETO_NETWORKSESSIONMANAGER_HPP
