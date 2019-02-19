//
// Created by Brett Chaldecott on 2019/02/17.
//

#include "keto/cli/TransactionLoader.hpp"

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/hex.h>

#include "keto/asn1/RDFObjectHelper.hpp"
#include "keto/asn1/RDFPredicateHelper.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/asn1/RDFModelHelper.hpp"
#include "keto/asn1/PrivateKeyHelper.hpp"

#include "keto/transaction_common/ChangeSetBuilder.hpp"
#include "keto/transaction_common/SignedChangeSetBuilder.hpp"

#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"

namespace keto {
namespace cli {


std::string TransactionLoader::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionLoader::TransactionLoader(const TransactionReader &transactionReader, const keto::crypto::KeyLoader &keyLoader) :
    transactionReader(transactionReader), keyLoader(keyLoader){

}


TransactionLoader::~TransactionLoader() {
}


keto::transaction_common::TransactionMessageHelperPtr TransactionLoader::load() {

    nlohmann::json transaction = transactionReader.getJsonData()["transaction"].get<nlohmann::json>();
    return load(transaction);

}

keto::transaction_common::TransactionMessageHelperPtr TransactionLoader::load(nlohmann::json transaction) {
    keto::asn1::HashHelper parentHash(transaction["parent"],keto::common::HEX);

    keto::asn1::HashHelper sourceAccount(transaction["account_hash"].get<std::string>().c_str(),keto::common::HEX);
    KETO_LOG_INFO << "Account hash : " << sourceAccount.getHash(keto::common::HEX);
    keto::asn1::NumberHelper numberHelper(
            atol(transaction["value"].get<std::string>().c_str()));
    std::shared_ptr<keto::chain_common::TransactionBuilder> transactionPtr =
            keto::chain_common::TransactionBuilder::createTransaction();
    transactionPtr->setParent(parentHash).setSourceAccount(sourceAccount)
            .setTargetAccount(sourceAccount).setValue(numberHelper);

    //std::cout << element << '\n';
    //std::cout << "Account hash : "  << element["account_hash"] << std::endl;
    //std::cout << "Public Key : "  << element["public_key"] << std::endl;
    for (nlohmann::json& action : transaction["actions"]) {
        nlohmann::json model = action["model"].get<nlohmann::json>();
        keto::asn1::RDFModelHelper modelHelper;
        for (nlohmann::json &element2 : model["rdf"]) {
            //std::cout << "Change set : "  << element2 << std::endl;
            for (nlohmann::json::iterator it = element2.begin(); it != element2.end(); ++it) {
                nlohmann::json predicate = it.value();
                keto::asn1::RDFSubjectHelper subjectHelper(it.key());
                KETO_LOG_INFO << "key : " << it.key() << " : [" << predicate << "]";
                for (nlohmann::json::iterator predIter = predicate.begin(); predIter != predicate.end(); ++predIter) {
                    keto::asn1::RDFPredicateHelper predicateHelper(predIter.key());
                    KETO_LOG_INFO << "key : " << predIter.key();
                    nlohmann::json contentWrapper = predIter.value();
                    KETO_LOG_INFO << "Content wrapper" << contentWrapper;
                    nlohmann::json jsonObj = contentWrapper.begin().value();
                    KETO_LOG_INFO << "Json object" << jsonObj;
                    keto::asn1::RDFObjectHelper objectHelper;
                    objectHelper.setDataType(jsonObj["datatype"].get<std::string>()).
                            setType(jsonObj["type"].get<std::string>()).
                            setValue(jsonObj["value"].get<std::string>());
                    KETO_LOG_INFO << "Add object to predicate";
                    predicateHelper.addObject(objectHelper);
                    subjectHelper.addPredicate(predicateHelper);
                }
                modelHelper.addSubject(subjectHelper);
            }
        }
        keto::asn1::AnyHelper anyModel(modelHelper);
        std::shared_ptr<keto::chain_common::ActionBuilder> actionBuilderPtr =
                keto::chain_common::ActionBuilder::createAction();
        actionBuilderPtr->setModel(anyModel);
        if (action.count("contractHash")) {
            keto::asn1::HashHelper contractHash(action["contractHash"].get<std::string>().c_str(), keto::common::HEX);
            actionBuilderPtr->setContract(contractHash);
        } else if (action.count("contractName")) {
            actionBuilderPtr->setContractName(action["contractName"].get<std::string>().c_str());
        }
        transactionPtr->addAction(actionBuilderPtr);
    }

    std::cout << "Memory data source private key " << transaction["private_key"].get<std::string>() << std::endl;
    std::cout << "Memory data source private key " << Botan::hex_encode(
            Botan::hex_decode_locked(transaction["private_key"].get<std::string>(),false),true) << std::endl;
    keto::asn1::PrivateKeyHelper privateKeyHelper(transaction["private_key"].get<std::string>(),keto::common::HEX);
    KETO_LOG_INFO << "Signed transaction builder";
    std::shared_ptr<keto::chain_common::SignedTransactionBuilder> signedTransBuild =
            keto::chain_common::SignedTransactionBuilder::createTransaction(
                    privateKeyHelper);
    KETO_LOG_INFO << "Sign transaction" << std::endl;
    signedTransBuild->setTransaction(transactionPtr).sign();
    keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelper(
            new keto::transaction_common::TransactionWrapperHelper(signedTransBuild->operator SignedTransaction*()));


    KETO_LOG_INFO << "Setup the transaction message";
    return keto::transaction_common::TransactionMessageHelperPtr(
            new keto::transaction_common::TransactionMessageHelper(transactionWrapperHelper));
}

}
}