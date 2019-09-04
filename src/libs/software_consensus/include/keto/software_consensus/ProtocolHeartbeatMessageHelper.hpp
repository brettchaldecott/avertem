//
// Created by Brett Chaldecott on 2019/01/22.
//

#ifndef KETO_PROTOCOLHEARTBEATMESSAGEHELPER_HPP
#define KETO_PROTOCOLHEARTBEATMESSAGEHELPER_HPP

#include <vector>
#include <memory>
#include <string>
#include <ctime>
#include <chrono>

#include "HandShake.pb.h"

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace software_consensus {

class ProtocolHeartbeatMessageHelper;
typedef std::shared_ptr<ProtocolHeartbeatMessageHelper> ProtocolHeartbeatMessageHelperPtr;


class ProtocolHeartbeatMessageHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ProtocolHeartbeatMessageHelper();
    ProtocolHeartbeatMessageHelper(long timestamp);
    ProtocolHeartbeatMessageHelper(const std::chrono::system_clock::time_point& timestamp);
    ProtocolHeartbeatMessageHelper(const keto::proto::ProtocolHeartbeatMessage& msg);
    ProtocolHeartbeatMessageHelper(const ProtocolHeartbeatMessageHelper& orig) = default;
    virtual ~ProtocolHeartbeatMessageHelper();

    ProtocolHeartbeatMessageHelper& setTimestamp(long timestamp);
    ProtocolHeartbeatMessageHelper& setTimestamp(const std::chrono::system_clock::time_point& timestamp);
    long getTimestamp();

    ProtocolHeartbeatMessageHelper& setNetworkSlot(int networkSlot);
    int getNetworkSlot() const;

    ProtocolHeartbeatMessageHelper& setElectionPublishSlot(int electionPublishSlot);
    int getElectionPublishSlot() const;

    ProtocolHeartbeatMessageHelper& setElectionSlot(int electionSlot);
    int getElectionSlot() const;

    ProtocolHeartbeatMessageHelper& setConfirmationSlot(int confirmationSlot);
    int getConfirmationSlot() const;


    ProtocolHeartbeatMessageHelper& setMsg(const keto::proto::ProtocolHeartbeatMessage& msg);
    keto::proto::ProtocolHeartbeatMessage getMsg();

    operator keto::proto::ProtocolHeartbeatMessage();
    operator std::string();


private:
    keto::proto::ProtocolHeartbeatMessage protocolHeartbeatMessage;
};


}
}

#endif //KETO_CONSENSUSACCEPTEDMESSAGEHELPER_HPP
