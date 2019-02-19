//
// Created by Brett Chaldecott on 2019/02/16.
//

#ifndef KETO_TRANSACTIONREADER_HPP
#define KETO_TRANSACTIONREADER_HPP

#include <string>
#include <nlohmann/json.hpp>
#include <boost/filesystem/path.hpp>

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace cli {


class TransactionReader {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };

    static std::string getSourceVersion();


    TransactionReader(const boost::filesystem::path& path);
    TransactionReader(const std::string& value);
    TransactionReader(const TransactionReader& orig) = default;
    virtual ~TransactionReader();

    nlohmann::json getJsonData();

private:
    nlohmann::json jsonData;
};


}
}


#endif //KETO_TRANSACTIONREADER_HPP
