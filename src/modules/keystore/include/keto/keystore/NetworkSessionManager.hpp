//
//  NetworkSessionKeys.hpp
//  0010_keto_keystore_module
//
//  Created by Brett Chaldecott on 2018/11/23.
//

#ifndef NetworkSessionKeys_h
#define NetworkSessionKeys_h

#include <string>
#include <map>
#include <memory>

#include "keto/module/ModuleInterface.hpp"
#include "keto/common/MetaInfo.hpp"

#include "keto/software_consensus/ConsensusHashGenerator.hpp"


namespace keto {
namespace keystore {
    
class NetworkSessionKeys;
typedef std::shared_ptr<NetworkSessionKeys> NetworkSessionKeysPtr;

    
class NetworkSessionManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();
    
    
    NetworkSessionKeys(const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator);
    NetworkSessionKeys(const NetworkSessionKeys& origin) = delete;
    virtual ~NetworkSessionKeys();
    
    
    // account service management methods
    static NetworkSessionKeysPtr init(const keto::software_consensus::ConsensusHashGeneratorPtr& consensusHashGenerator);
    static void fin();
    static NetworkSessionKeysPtr getInstance();
    
    
    // set the session key
    
private:
    keto::software_consensus::ConsensusHashGeneratorPtr consensusHashGenerator;
    
    
    
}
    
    
}
}



#endif /* NetworkSessionKeys_h */
