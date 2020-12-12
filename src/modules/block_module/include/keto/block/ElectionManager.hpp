//
// Created by Brett Chaldecott on 2019-08-23.
//

#ifndef KETO_ELECTIONMANAGER_HPP
#define KETO_ELECTIONMANAGER_HPP

#include <string>
#include <thread>
#include <memory>
#include <deque>
#include <mutex>
#include <map>
#include <condition_variable>

#include "keto/common/MetaInfo.hpp"
#include "keto/event/Event.hpp"

#include "keto/asn1/RDFObjectHelper.hpp"
#include "keto/asn1/RDFPredicateHelper.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/asn1/RDFModelHelper.hpp"

#include "keto/election_common/ElectionResultMessageProtoHelper.hpp"
#include "keto/software_consensus/ProtocolHeartbeatMessageHelper.hpp"
#include "keto/election_common/SignedElectNodeHelper.hpp"

namespace keto {
namespace block {

class ElectionManager;
typedef std::shared_ptr<ElectionManager> ElectionManagerPtr;

class ElectionManager {
public:
    enum State {
        PROCESSING,
        ELECT,
        PUBLISH,
        CONFIRMATION,
    };

    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class Elector {
    public:
        Elector(const keto::asn1::HashHelper& account, const std::string& type);
        Elector(const Elector& orig) = delete;
        virtual ~Elector();


        keto::asn1::HashHelper getAccount();
        std::string getType();
        bool isSet();
        keto::election_common::ElectionResultMessageProtoHelperPtr getElectionResult();
        void setElectionResult(
                const keto::election_common::ElectionResultMessageProtoHelperPtr& result);

    private:
        keto::asn1::HashHelper account;
        std::string type;
        keto::election_common::ElectionResultMessageProtoHelperPtr electionResultMessageProtoHelperPtr;
    };
    typedef std::shared_ptr<Elector> ElectorPtr;

    class Window {
    public:
        Window(const std::vector<keto::asn1::HashHelper>& tangles);
        Window(const Window& orig) = delete;
        virtual ~Window();

        std::vector<keto::asn1::HashHelper> getTangles();
        void addTangle(const keto::asn1::HashHelper& tangle);
        bool isGrowing();
        bool equals(const std::vector<keto::asn1::HashHelper>& tangles);
    private:
        std::vector<keto::asn1::HashHelper> tangles;
        bool growing;
    };
    typedef std::shared_ptr<Window> WindowPtr;

    class WindowManager {
    public:
        WindowManager();
        WindowManager(const WindowManager& orig) = delete;
        virtual ~WindowManager();

        void addWindow(const std::vector<keto::asn1::HashHelper>& tangles);
        bool hasNewWindows();
        std::vector<keto::asn1::HashHelper> getAllTangles();
        void mergeActiveTangles(const std::vector<keto::asn1::HashHelper>& tangles);
        void activate();
        void election();
        void clear();

        std::vector<WindowPtr> getWindows();

    private:
        std::vector<WindowPtr> newWindow;
        std::vector<WindowPtr> currentWindow;

        void insertTangles(std::vector<keto::asn1::HashHelper>& targetTangles, const std::vector<keto::asn1::HashHelper>& tangles);
    };
    typedef std::shared_ptr<WindowManager> WindowManagerPtr;

    ElectionManager();
    ElectionManager(const ElectionManager& orig) = delete;
    virtual ~ElectionManager();

    static ElectionManagerPtr init();
    static void fin();
    static ElectionManagerPtr getInstance();


    keto::event::Event consensusHeartbeat(const keto::event::Event& event);
    keto::event::Event electRpcRequest(const keto::event::Event& event);
    keto::event::Event electRpcResponse(const keto::event::Event& event);
    keto::event::Event electRpcProcessPublish(const keto::event::Event& event);
    keto::event::Event electRpcProcessConfirmation(const keto::event::Event& event);

private:
    std::recursive_mutex classMutex;
    ElectionManager::State state;
    keto::asn1::HashHelper faucetAccount;
    std::map<std::vector<uint8_t>,ElectorPtr> accountElectionResult;
    int responseCount;
    WindowManagerPtr windowManagerPtr;
    //std::vector<keto::asn1::HashHelper> nextWindow;
    //std::vector<keto::asn1::HashHelper> currentWindow;
    std::set<std::vector<uint8_t>> electedAccounts;

    void invokeElection(const std::string& event, const std::string& type);
    void publishElection();
    void confirmElection();
    std::vector<std::vector<uint8_t>> listAccounts();

    keto::election_common::SignedElectNodeHelperPtr generateSignedElectedNode(std::vector<std::vector<uint8_t>>& accounts);
    void generateTransaction(const keto::election_common::SignedElectNodeHelperPtr& signedElectNodeHelperPtr,
            const std::vector<keto::asn1::HashHelper>& tangles);
    keto::asn1::RDFPredicateHelper buildPredicate(const std::string& predicate,
                                                                   const std::string& type,
                                                                   const std::string& datatype,
                                                                   const std::string& value);

};


}
}

#endif //KETO_ELECTIONMANAGER_HPP
