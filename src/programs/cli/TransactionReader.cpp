//
// Created by Brett Chaldecott on 2019/02/16.
//

#include <iostream>
#include <fstream>

#include "keto/cli/TransactionReader.hpp"


namespace keto {
namespace cli {

std::string TransactionReader::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

TransactionReader::TransactionReader(const boost::filesystem::path& path) {
    std::ifstream ifs(path.string());
    ifs >> this->jsonData;
}

TransactionReader::TransactionReader(const std::string& value) {
    this->jsonData = nlohmann::json::parse(value);
}

TransactionReader::~TransactionReader() {

}

nlohmann::json TransactionReader::getJsonData() {
    return this->jsonData;
}


}
}