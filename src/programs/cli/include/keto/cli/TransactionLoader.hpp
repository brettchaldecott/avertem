//
// Created by Brett Chaldecott on 2019/02/17.
//

#ifndef KETO_TRANSACTIONLOADER_HPP
#define KETO_TRANSACTIONLOADER_HPP

#include <string>

#include "keto/cli/TransactionReader.hpp"
#include "keto/crypto/KeyLoader.hpp"

#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace cli {


class TransactionLoader {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();

    TransactionLoader(const TransactionReader& transactionReader, const keto::crypto::KeyLoaderPtr& keyLoaderPtr);
    TransactionLoader(const TransactionLoader& orig) = default;
    virtual ~TransactionLoader();


    keto::transaction_common::TransactionMessageHelperPtr load();

private:
    TransactionReader transactionReader;
    keto::crypto::KeyLoaderPtr keyLoader;

    keto::transaction_common::TransactionMessageHelperPtr load(nlohmann::json transaction);
};


}
}

#endif //KETO_TRANSACTIONLOADER_HPP
