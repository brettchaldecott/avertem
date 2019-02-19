//
// Created by Brett Chaldecott on 2019/02/18.
//

#ifndef KETO_NETWORKFEESMANAGER_HPP
#define KETO_NETWORKFEESMANAGER_HPP

#include <string>
#include <memory>
#include <vector>

#include "BlockChain.pb.h"

#include "keto/event/Event.hpp"

#include "keto/obfuscate/MetaString.hpp"

#include "keto/transaction_common/FeeInfoMsgProtoHelper.hpp"

namespace keto {
namespace block {

class NetworkFeeManager;
typedef std::shared_ptr<NetworkFeeManager> NetworkFeeManagerPtr;

class NetworkFeeManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();


    NetworkFeeManager();
    NetworkFeeManager(const NetworkFeeManager& orig) = delete;
    virtual ~NetworkFeeManager();

    static NetworkFeeManagerPtr getInstance();
    static NetworkFeeManagerPtr init();
    static void fin();

    void load();
    void clear();

    keto::transaction_common::FeeInfoMsgProtoHelperPtr getFeeInfo();
    void setFeeInfo(const keto::transaction_common::FeeInfoMsgProtoHelperPtr& feeInfoMsgProtoHelperPtr);

    keto::event::Event getNetworkFeeInfo(const keto::event::Event& event);
    keto::event::Event setNetworkFeeInfo(const keto::event::Event& event);

private:
    keto::transaction_common::FeeInfoMsgProtoHelperPtr feeInfoMsgProtoHelperPtr;
};



}
}


#endif //KETO_NETWORKFEESERVICE_HPP
