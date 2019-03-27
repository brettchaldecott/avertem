//
// Created by Brett Chaldecott on 2019/03/07.
//

#ifndef KETO_ACCOUNTSPARQLQUERYHELPER_HPP
#define KETO_ACCOUNTSPARQLQUERYHELPER_HPP

#include <string>
#include <vector>
#include <map>


#include "keto/asn1/HashHelper.hpp"


namespace keto {
namespace account_query {

typedef std::map<std::string,std::string> ResultMap;
typedef std::vector<ResultMap> ResultVectorMap;

class AccountSparqlQueryHelper {
public:

    AccountSparqlQueryHelper(const std::string& action, const keto::asn1::HashHelper& accountHash, const std::string& query);
    AccountSparqlQueryHelper(const AccountSparqlQueryHelper& orig) = default;
    virtual ~AccountSparqlQueryHelper();

    ResultVectorMap execute();

private:
    std::string action;
    keto::asn1::HashHelper accountHash;
    std::string query;
};


}
}



#endif //KETO_ACCOUNTSPARQLQUERYHELPER_HPP
