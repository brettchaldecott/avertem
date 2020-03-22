//
// Created by Brett Chaldecott on 2019-10-07.
//

#ifndef KETO_WAVMSESSIONTRANSACTIONBUILDER_HPP
#define KETO_WAVMSESSIONTRANSACTIONBUILDER_HPP


#include <memory>
#include <string>
#include <cstdlib>

#include "Sandbox.pb.h"

#include "keto/asn1/ChangeSetHelper.hpp"
#include "keto/asn1/RDFSubjectHelper.hpp"
#include "keto/asn1/RDFModelHelper.hpp"
#include "keto/asn1/RDFObjectHelper.hpp"
#include "keto/asn1/NumberHelper.hpp"
#include "keto/wavm_common/RDFMemorySession.hpp"
#include "keto/wavm_common/WavmSession.hpp"

#include "keto/transaction_common/TransactionProtoHelper.hpp"

#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/ActionBuilder.hpp"

#include "keto/crypto/KeyLoader.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace wavm_common {

class WavmSessionTransactionBuilder;
typedef std::shared_ptr<WavmSessionTransactionBuilder> WavmSessionTransactionBuilderPtr;

class WavmSessionTransactionBuilder {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    class WavmSessionModelBuilder {
    public:
        WavmSessionModelBuilder();
        WavmSessionModelBuilder(const WavmSessionModelBuilder& orig) = delete;
        virtual ~WavmSessionModelBuilder();

        virtual std::string getType() = 0;
    };
    typedef std::shared_ptr<WavmSessionModelBuilder> WavmSessionModelBuilderPtr;

    class WavmSessionRDFModelBuilder : public WavmSessionModelBuilder {
    public:
        WavmSessionRDFModelBuilder();
        WavmSessionRDFModelBuilder(const WavmSessionRDFModelBuilder& orig) = delete;
        virtual ~WavmSessionRDFModelBuilder();

        std::string getRequestStringValue(const std::string& subjectUrl, const std::string& predicateUrl);
        void setRequestStringValue(const std::string& subjectUrl, const std::string& predicateUrl, const std::string& value);

        long getRequestLongValue(const std::string& subjectUrl, const std::string& prediateUrl);
        void setRequestLongValue(const std::string& subjectUrl, const std::string& prediateUrl, const long& value);

        float getRequestFloatValue(const std::string& subjectUrl, const std::string& prediateUrl);
        void setRequestFloatValue(const std::string& subjectUrl, const std::string& prediateUrl, const float& value);

        bool getRequestBooleanValue(const std::string& subjectUrl, const std::string& prediateUrl);
        void setRequestBooleanValue(const std::string& subjectUrl, const std::string& prediateUrl, const bool& value);

        virtual std::string getType();

        operator keto::asn1::AnyHelper() const;
    private:
        keto::asn1::RDFModelHelper modelHelper;

        keto::asn1::RDFSubjectHelperPtr getSubject(const std::string& subjectUrl);
        keto::asn1::RDFPredicateHelperPtr getPredicate(
                keto::asn1::RDFSubjectHelperPtr subject, const std::string& predicateUrl);
        void addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                           const std::string& value);
        void addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                           const long value);
        void addModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                           const float value);
        void addBooleanModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                                  const bool value);
        void addDateTimeModelEntry(const std::string& subjectUrl, const std::string predicateUrl,
                                   const time_t value);

    };
    typedef std::shared_ptr<WavmSessionRDFModelBuilder> WavmSessionRDFModelBuilderPtr;


    class WavmSessionActionBuilder {
    public:
        WavmSessionActionBuilder(int id, const std::string& modelType);
        WavmSessionActionBuilder(const WavmSessionActionBuilder& orig) = delete;
        virtual ~WavmSessionActionBuilder();

        // this id is only used for tracking the actions created for the
        int getId();

        keto::asn1::HashHelper getContract();
        void setContract(const keto::asn1::HashHelper& contract);

        std::string getContractName();
        void setContractName(const std::string& contractName);

        std::string getModelType();
        WavmSessionModelBuilderPtr getModel();

        operator keto::chain_common::ActionBuilderPtr() const;
    private:
        int id;
        keto::chain_common::ActionBuilderPtr actionBuilderPtr;
        WavmSessionModelBuilderPtr wavmSessionModelBuilderPtr;
    };
    typedef std::shared_ptr<WavmSessionActionBuilder> WavmSessionActionBuilderPtr;

    WavmSessionTransactionBuilder(int id, const keto::crypto::KeyLoaderPtr& keyLoaderPtr);
    WavmSessionTransactionBuilder(int id, const keto::asn1::HashHelper& hashHelper,
                                                                 const keto::crypto::KeyLoaderPtr& keyLoaderPtr);
    WavmSessionTransactionBuilder(const WavmSessionTransactionBuilder& orig) = delete;
    virtual ~WavmSessionTransactionBuilder();

    int getId();

    void setValue(const keto::asn1::NumberHelper& numberHelper);
    keto::asn1::NumberHelper getValue();

    void setParent(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getParent();

    void setSourceAccount(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getSourceAccount();

    void setTargetAccount(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getTargetAccount();

    void setTransactionSignator(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getTransactionSignator();

    void setCreatorId(const keto::asn1::HashHelper& hashHelper);
    keto::asn1::HashHelper getCreatorId();

    WavmSessionActionBuilderPtr createAction(const std::string& modelType);
    WavmSessionActionBuilderPtr getAction(const int& id);

    void submit();
    void submitWithStatus(const std::string& status = "INIT");

private:
    int id;
    keto::chain_common::TransactionBuilderPtr transactionBuilderPtr;
    keto::crypto::KeyLoaderPtr keyLoaderPtr;
    std::vector<WavmSessionActionBuilderPtr> actions;
    bool submitted;

};

}
}



#endif //KETO_WAVMSESSIONTRANSACTIONBUILDER_HPP
