//
// Created by Brett Chaldecott on 2019/01/28.
//

#ifndef KETO_MASTERKEYMANAGER_HPP
#define KETO_MASTERKEYMANAGER_HPP

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>

#include <nlohmann/json.hpp>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/rsa.h>
#include <botan/auto_rng.h>

#include "KeyStore.pb.h"

#include "keto/environment/Config.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/software_consensus/ConsensusHashGenerator.hpp"

#include "keto/crypto/KeyLoader.hpp"
#include "keto/event/Event.hpp"
#include "keto/keystore/Constants.hpp"
#include "keto/key_store_db/KeyStoreDB.hpp"
#include "keto/rpc_protocol/NetworkKeysHelper.hpp"

namespace keto {
namespace keystore {

class MasterKeyManager;
typedef std::shared_ptr<MasterKeyManager> MasterKeyManagerPtr;

class MasterKeyManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    class MasterKeyListEntry {
    public:
        MasterKeyListEntry();
        MasterKeyListEntry(const std::string& json);
        MasterKeyListEntry(const MasterKeyListEntry& orig) = default;
        virtual ~MasterKeyListEntry();

        void addKey(const std::string& hash);
        std::vector<std::string> getKeys();
        std::string getJson();

    private:
        std::vector<std::string> keys;

    };
    typedef std::shared_ptr<MasterKeyListEntry> MasterKeyListEntryPtr;

    class Session {
    public:
        Session() {}
        Session(const Session& orig) = delete;
        virtual ~Session() {}

        virtual void initSession();
        virtual void clearSession();


        virtual bool isMaster() const = 0;

        // methods to get and set the master key
        virtual keto::event::Event getMasterKey(const keto::event::Event& event) = 0;
        virtual keto::event::Event setMasterKey(const keto::event::Event& event) = 0;


        // this method returns the wrapping keys
        virtual keto::event::Event getWrappingKeys(const keto::event::Event& event) = 0;
        virtual keto::event::Event setWrappingKeys(const keto::event::Event& event) = 0;
    };
    typedef std::shared_ptr<Session> SessionPtr;

    class MasterSession : public Session {
    public:
        MasterSession(std::shared_ptr<keto::environment::Config> config);
        MasterSession(const MasterSession& orig) = delete;
        virtual ~MasterSession();

        virtual void initSession();
        virtual void clearSession();


        bool isMaster() const;

        // methods to get and set the master key
        keto::event::Event getMasterKey(const keto::event::Event& event);
        keto::event::Event setMasterKey(const keto::event::Event& event);


        // this method returns the wrapping keys
        keto::event::Event getWrappingKeys(const keto::event::Event& event);
        keto::event::Event setWrappingKeys(const keto::event::Event& event);

    private:
        keto::key_store_db::KeyStoreDBPtr keyStoreDBPtr;
        std::shared_ptr<keto::crypto::KeyLoader> masterKeyLock;
        MasterKeyListEntryPtr masterKeyList;
        MasterKeyListEntryPtr masterWrapperKeyList;

        void initMasterKeys();
        MasterKeyListEntryPtr generateKeys(size_t number,keto::key_store_db::OnionKeys onionKeys);
        void loadKeys(MasterKeyListEntryPtr keyList, keto::rpc_protocol::NetworkKeysHelper& networkKeysHelper,
                      keto::key_store_db::OnionKeys& onionKeys);

    };

    class SlaveSession : public Session {
    public:
        SlaveSession();
        SlaveSession(const SlaveSession& orig) = delete;
        virtual ~SlaveSession();

        bool isMaster() const;

        // methods to get and set the master key
        keto::event::Event getMasterKey(const keto::event::Event& event);
        keto::event::Event setMasterKey(const keto::event::Event& event);


        // this method returns the wrapping keys
        keto::event::Event getWrappingKeys(const keto::event::Event& event);
        keto::event::Event setWrappingKeys(const keto::event::Event& event);
    private:
        // slave keys
        std::mutex classMutex;
        bool slaveMaster;
        keto::proto::NetworkKeysWrapper slaveMasterKeys;
        bool slaveWrapper;
        keto::proto::NetworkKeysWrapper slaveWrapperKeys;
    };


    MasterKeyManager();
    MasterKeyManager(const MasterKeyManager& orig) = delete;
    virtual ~MasterKeyManager();

    static MasterKeyManagerPtr init();
    static void fin();
    static MasterKeyManagerPtr getInstance();

    void initSession();
    void clearSession();


    // methods to get and set the master key
    keto::event::Event getMasterKey(const keto::event::Event& event);
    keto::event::Event setMasterKey(const keto::event::Event& event);


    // this method returns the wrapping keys
    keto::event::Event getWrappingKeys(const keto::event::Event& event);
    keto::event::Event setWrappingKeys(const keto::event::Event& event);

    /**
     * This method returns the master reference.
     *
     * @return TRUE if this is the master
     */
    bool isMaster() const;


    keto::event::Event isMaster(const keto::event::Event& event) const;

private:
    SessionPtr sessionPtr;


};


}
}


#endif //KETO_MASTERKEYMANAGER_HPP
