//
// Created by Brett Chaldecott on 2019/02/08.
//

#ifndef KETO_BLOCKCHAINCALLBACK_HPP
#define KETO_BLOCKCHAINCALLBACK_HPP


#include <string>
#include <memory>

#include "SignedBlock.h"

#include "keto/asn1/HashHelper.hpp"

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace block_db {

class BlockChainCallback {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };


    virtual void applyDirtyTransaction(const keto::asn1::HashHelper chainId, const TransactionWrapper_t& transactionWrapper) const {};
    virtual void prePersistBlock(const keto::asn1::HashHelper chainId, const SignedBlock& signedBlock) const {};
    virtual void prePersistTransaction(const keto::asn1::HashHelper chainId, const SignedBlock& signedBlock, const TransactionWrapper_t& transactionWrapper) const {};
    virtual void postPersistTransaction(const keto::asn1::HashHelper chainId, const SignedBlock& signedBlock, const TransactionWrapper_t& transactionWrapper) const {};
    virtual void postPersistBlock(const keto::asn1::HashHelper chainId, SignedBlock& signedBlock) const {};

    virtual bool producerEnding() const = 0;
};

}
}


#endif //KETO_BLOCKCHAINCALLBACK_HPP
