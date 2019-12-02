/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constants.hpp
 * Author: ubuntu
 *
 * Created on January 22, 2018, 3:36 PM
 */

#ifndef CONSENSUS_MODULE_CONSTANTS_HPP
#define CONSENSUS_MODULE_CONSTANTS_HPP

#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace consensus_module {


class Constants {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    Constants() = delete;
    Constants(const Constants& orig) = delete;
    virtual ~Constants() = delete;
    
    // keys for server
    static constexpr const char* PRIVATE_KEY    = "server-private-key";
    static constexpr const char* PUBLIC_KEY     = "server-public-key";
    static constexpr const char* CONSENSUS_KEY     = "consensus-keys";

    static constexpr const char* NETWORK_SESSION_LENGTH_CONFIGURATION     = "network_session_length";
    static constexpr const int NETWORK_SESSION_LENGTH_DEFAULT     = 120;
    static constexpr const char* NETWORK_CONSENSUS_HEARTBEAT_CONFIGURATION     = "network_consensus_heartbeat";
    static constexpr const int NETWORK_CONSENSUS_HEARTBEAT_DEFAULT     = 60;
    static constexpr const char* NETWORK_CONSENSUS_HEARTBEAT_NETWORK_ELECTION_SLOT     = "network_consensus_election_slot";
    static constexpr const int NETWORK_CONSENSUS_HEARTBEAT_ELECTION_PUBLISH_SLOT_DEFAULT     = 6;
    static constexpr const char* NETWORK_CONSENSUS_HEARTBEAT_NETWORK_ELECTION_PUBLISH_SLOT     = "network_consensus_election_publish_slot";
    static constexpr const int NETWORK_CONSENSUS_HEARTBEAT_ELECTION_SLOT_DEFAULT     = 5;
    static constexpr const char* NETWORK_CONSENSUS_HEARTBEAT_NETWORK_CONFIRMATION_SLOT     = "network_consensus_confirmation_slot";
    static constexpr const int NETWORK_CONSENSUS_HEARTBEAT_CONFIRMATION_SLOT_DEFAULT     = 7;

};

}
}

#endif /* CONSTANTS_HPP */

