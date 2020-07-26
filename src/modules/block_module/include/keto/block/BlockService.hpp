/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockService.hpp
 * Author: ubuntu
 *
 * Created on March 8, 2018, 3:04 AM
 */

#ifndef BLOCKSERVICE_HPP
#define BLOCKSERVICE_HPP

#include <memory>
#include <mutex>
#include <map>

#include "keto/event/Event.hpp"
#include "keto/common/MetaInfo.hpp"

namespace keto {
namespace block {

typedef std::vector<uint8_t> AccountHashVector;
    
class BlockService {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    
    static std::string getSourceVersion();

    class SignedBlockWrapperCache;
    typedef std::shared_ptr<SignedBlockWrapperCache> SignedBlockWrapperCachePtr;
    class SignedBlockWrapperCache {
    public:
        SignedBlockWrapperCache();
        SignedBlockWrapperCache(const SignedBlockWrapperCache& orig) = delete;
        virtual ~SignedBlockWrapperCache();

        bool checkCache(const keto::asn1::HashHelper& signedBlockWrapperCacheHash);
    private:
        std::mutex classMutex;
        std::set<std::string> cacheLookup;
        std::deque<std::string> cacheHistory;
    };


    BlockService();
    BlockService(const BlockService& orig) = delete;
    virtual ~BlockService();
    
    static std::shared_ptr<BlockService> init();
    static void fin();
    static std::shared_ptr<BlockService> getInstance();
    
    bool genesis();
    void sync();

    keto::event::Event persistBlockMessage(const keto::event::Event& event);
    keto::event::Event blockMessage(const keto::event::Event& event);
    keto::event::Event requestBlockSync(const keto::event::Event& event);
    keto::event::Event processBlockSyncResponse(const keto::event::Event& event);
    keto::event::Event processRequestBlockSyncRetry(const keto::event::Event& event);

    keto::event::Event getAccountBlockTangle(const keto::event::Event& event);

    // query methods
    keto::event::Event getBlocks(const keto::event::Event& event);
    keto::event::Event getBlockTransactions(const keto::event::Event& event);
    keto::event::Event getTransaction(const keto::event::Event& event);
    keto::event::Event getAccountTransactions(const keto::event::Event& event);
private:
    std::mutex classMutex;

    SignedBlockWrapperCachePtr signedBlockWrapperCachePtr;
    std::map<AccountHashVector,std::mutex> accountLocks;
    
    std::mutex& getAccountLock(const AccountHashVector& accountHash);
    
};


}
}

#endif /* BLOCKSERVICE_HPP */

