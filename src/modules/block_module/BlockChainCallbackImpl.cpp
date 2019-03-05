//
// Created by Brett Chaldecott on 2019/02/08.
//

#include "keto/block/BlockChainCallbackImpl.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/transaction_common/AccountTransactionInfoProtoHelper.hpp"


namespace keto {
namespace block {

BlockChainCallbackImpl::BlockChainCallbackImpl() {

}

BlockChainCallbackImpl::~BlockChainCallbackImpl() {

}

void BlockChainCallbackImpl::applyDirtyTransaction(const keto::asn1::HashHelper chainId, const TransactionWrapper_t& transactionWrapper) const {
    // we call onto the accounting system to persist the processed transaction
    keto::transaction_common::AccountTransactionInfoProtoHelper accountTransactionInfoProtoHelper(
            chainId,transactionWrapper);
    keto::proto::AccountTransactionInfo accountTransactionInfo = accountTransactionInfoProtoHelper;
    accountTransactionInfo = keto::server_common::fromEvent<keto::proto::AccountTransactionInfo>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::AccountTransactionInfo>(
                    keto::server_common::Events::APPLY_ACCOUNT_DIRTY_TRANSACTION_MESSAGE,accountTransactionInfo)));
}


void BlockChainCallbackImpl::prePersistBlock(const keto::asn1::HashHelper chainId, const SignedBlock& signedBlock) const {

}


void BlockChainCallbackImpl::prePersistTransaction(const keto::asn1::HashHelper chainId, const SignedBlock& signedBlock, const TransactionWrapper& transactionWrapper) const {
    keto::transaction_common::AccountTransactionInfoProtoHelper accountTransactionInfoProtoHelper(chainId,signedBlock.hash,transactionWrapper);
    keto::proto::AccountTransactionInfo accountTransactionInfo = accountTransactionInfoProtoHelper;
    accountTransactionInfo = keto::server_common::fromEvent<keto::proto::AccountTransactionInfo>(
                keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::AccountTransactionInfo>(
                        keto::server_common::Events::APPLY_ACCOUNT_TRANSACTION_MESSAGE,accountTransactionInfo)));
}

void BlockChainCallbackImpl::postPersistTransaction(const keto::asn1::HashHelper chainId, const SignedBlock& signedBlock, const TransactionWrapper& transactionWrapper) const {

}

void BlockChainCallbackImpl::postPersistBlock(const keto::asn1::HashHelper chainId, SignedBlock& signedBlock) const {

}

}
}