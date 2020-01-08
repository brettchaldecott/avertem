//
// Created by Brett Chaldecott on 2019/04/15.
//

#ifndef KETO_RDFQUERYPARSER_HPP
#define KETO_RDFQUERYPARSER_HPP


#include <string>
#include <vector>

#include <rasqal/rasqal.h>
#include "rasqal.h"
#include "raptor.h"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace rdf_utils {


class RDFQueryParser {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    RDFQueryParser(const std::string& sparql);
    RDFQueryParser(const std::string& sparql, const std::string& account);
    RDFQueryParser(const std::string& sparql, const std::vector<uint8_t>& account);
    RDFQueryParser(const RDFQueryParser& orig) = default;
    virtual ~RDFQueryParser();

    bool isValidQuery();

    std::string getQuery();

    operator const unsigned char *();

private:
    std::string sparql;
    std::string account;
    rasqal_world *world;
    rasqal_query *query;
    rasqal_variables_table* variablesTable;



    void processPattern();

    bool containsUri(raptor_sequence* patternTrippleSequence, const std::string& uri);
    std::vector<rasqal_literal*> getSubjects(raptor_sequence* patternTrippleSequence);
    bool isExcludedPredicate(rasqal_triple* patternTrippleSequence);
    std::vector<rasqal_variable*> getVariables(raptor_sequence* patternTrippleSequence);
    void getVariable(rasqal_literal* literal, std::vector<rasqal_variable*>& variables, std::set<std::string>& variableEntries);
    void addTripplePattern(
            raptor_sequence* patternTrippleSequence, const std::string& predicateUri, const std::string& objectUri);

    std::string getLiteralUri(rasqal_literal* variable);
    std::string getLiteralString(rasqal_literal* variable);
    std::string getLiteralBoolean(rasqal_literal* variable);
    std::string getLiteralInteger(rasqal_literal* variable);
    std::string getLiteralFloat(rasqal_literal* variable);
    std::string getLiteralDecimal(rasqal_literal* variable);
    std::string getLiteralDateTime(rasqal_literal* variable);
    std::string getLiteralDate(rasqal_literal* variable);
    std::string getVariable(rasqal_literal* variable);
    std::string getLiteralInfo(rasqal_literal* variable);
};


}
}


#endif //KETO_RDFQUERYPARSER_HPP
