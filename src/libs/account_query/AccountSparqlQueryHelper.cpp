//
// Created by Brett Chaldecott on 2019/03/07.
//

#include "Sparql.pb.h"

#include "keto/account_query/AccountSparqlQueryHelper.hpp"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"
#include "keto/server_common/Constants.hpp"


namespace keto {
namespace account_query {


AccountSparqlQueryHelper::AccountSparqlQueryHelper(const std::string& action, const keto::asn1::HashHelper& accountHash, const std::string& query) :
    action(action), accountHash(accountHash), query(query) {

}

AccountSparqlQueryHelper::~AccountSparqlQueryHelper() {
}

ResultVectorMap AccountSparqlQueryHelper::execute() {
    keto::proto::SparqlResultSetQuery sparqlResultSetQuery;
    sparqlResultSetQuery.set_account_hash(accountHash);
    sparqlResultSetQuery.set_query(query);

    keto::proto::SparqlResultSet sparqlResultSet = keto::server_common::fromEvent<keto::proto::SparqlResultSet>(
            keto::server_common::processEvent(keto::server_common::toEvent<keto::proto::SparqlResultSetQuery>(
                    this->action,sparqlResultSetQuery)));


    ResultVectorMap resultVectorMap;
    for (int rowIndex = 0; rowIndex < sparqlResultSet.rows_size(); rowIndex++) {
        const keto::proto::SparqlRow& row = sparqlResultSet.rows(rowIndex);
        ResultMap resultMap;
        for (int columnIndex = 0; columnIndex < row.entries_size(); columnIndex++) {
            const keto::proto::SparqlRowEntry& entry = row.entries(columnIndex);
            std::cout << "Add the entry [" << entry.key() << "][" << entry.value() << "]" << std::endl;
            resultMap[entry.key()] = entry.value();
        }
        resultVectorMap.push_back(resultMap);
    }

    std::cout << "Return the query results" << std::endl;
    return resultVectorMap;
}


}
}