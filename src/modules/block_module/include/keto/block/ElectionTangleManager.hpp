//
// Created by Brett Chaldecott on 2021/10/23.
//

#ifndef KETO_ELECTIONTANGLEMANAGER_HPP
#define KETO_ELECTIONTANGLEMANAGER_HPP

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

class ElectionTangleManager;
typedef std::shared_ptr<ElectionTangleManager> ElectionTangleManagerPtr;

class ElectionTangleManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    ElectionTangleManager() {}
    ElectionTangleManager(const ElectionTangleManager& orig) = delete;
    virtual ~ElectionTangleManager();

    static ElectionTangleManagerPtr init();
    static ElectionTangleManagerPtr getInstance();
    static ElectionTangleManagerPtr fin();

    virtual void beginElection() = 0;
    virtual void confirmElection(const std::vector<keto::asn1::HashHelper>& tangles) = 0;
    virtual std::vector<keto::asn1::HashHelper> getInactiveTangles() = 0;

private:

};


class MasterElectionTangleManager : public ElectionTangleManager {
public:
    MasterElectionTangleManager();
    MasterElectionTangleManager(const MasterElectionTangleManager& orig) = delete;
    virtual ~MasterElectionTangleManager();

    void beginElection();
    void confirmElection(const std::vector<keto::asn1::HashHelper>& tangles);
    std::vector<keto::asn1::HashHelper> getInactiveTangles();
private:
    std::vector<keto::asn1::HashHelper> currentInactiveTangles;
};

class SlaveElectionTangleManager : public ElectionTangleManager {
public:
    SlaveElectionTangleManager() {}
    SlaveElectionTangleManager(const SlaveElectionTangleManager& orig) = delete;
    virtual ~SlaveElectionTangleManager() {}

    void beginElection() {};
    void confirmElection(const std::vector<keto::asn1::HashHelper>& tangles) {};
    std::vector<keto::asn1::HashHelper> getInactiveTangles() { return std::vector<keto::asn1::HashHelper>();};
private:

};

}
}


#endif //KETO_ELECTIONTANGLEMANAGER_HPP
