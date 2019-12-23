//
// Created by Brett Chaldecott on 2019/04/15.
//

#include <set>
#include <sstream>

#include <raptor2.h>
#include <rasqal/rasqal.h>

#include <botan/hex.h>

#include "keto/rdf_utils/RDFQueryParser.hpp"
#include "keto/rdf_utils/Constants.hpp"
#include "keto/rdf_utils/Exception.hpp"
#include "librdf_internal.h"


namespace keto {
namespace rdf_utils {

int rasqal_graph_pattern_visit_fn(rasqal_query *query, rasqal_graph_pattern *gp,
                                  void *user_data) {
    raptor_sequence* patternTrippleSequence = (raptor_sequence*)user_data;
    gp->end_column = gp->end_column + (raptor_sequence_size(patternTrippleSequence) -
        raptor_sequence_size(gp->triples));
    while(true) {
        rasqal_triple* temp = (rasqal_triple*)raptor_sequence_pop(gp->triples);
        if (!temp) {
            break;
        }
        rasqal_free_triple(temp);
    }
    gp->triples = patternTrippleSequence;
    return 0;
}

std::string RDFQueryParser::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

RDFQueryParser::RDFQueryParser(const std::string& sparql) : sparql(sparql){
    this->world=rasqal_new_world();
    rasqal_world_open(world);
    this->query=rasqal_new_query(world, keto::rdf_utils::Constants::SPARQL, NULL);


    rasqal_query_prepare(query, (const unsigned char *)sparql.c_str(), NULL);



}

RDFQueryParser::RDFQueryParser(const std::string& sparql, const std::vector<uint8_t>& account) : sparql(sparql) {

    this->account = Botan::hex_encode(account,true);

    this->world=rasqal_new_world();
    rasqal_world_open(world);
    this->query=rasqal_new_query(world, keto::rdf_utils::Constants::SPARQL, NULL);

    rasqal_query_prepare(query, (const unsigned char *)sparql.c_str(), NULL);

    processPattern(query);
}

RDFQueryParser::RDFQueryParser(const std::string& sparql, const std::string& account) : sparql(sparql), account(account){
    this->world=rasqal_new_world();
    rasqal_world_open(world);
    this->query=rasqal_new_query(world, keto::rdf_utils::Constants::SPARQL, NULL);

    rasqal_query_prepare(query, (const unsigned char *)sparql.c_str(), NULL);

    processPattern(query);

}

RDFQueryParser::~RDFQueryParser() {

    rasqal_free_query(this->query);
    rasqal_free_world(this->world);
}

bool RDFQueryParser::isValidQuery() {
    return rasqal_query_get_verb(query) == RASQAL_QUERY_VERB_SELECT;
}

std::string RDFQueryParser::getQuery() {
    return sparql;
}

RDFQueryParser::operator const unsigned char *() {
    return (const unsigned char *)getQuery().c_str();
}


void RDFQueryParser::processPattern(rasqal_query *query) {
    KETO_LOG_DEBUG << "Get the pattern";
    rasqal_graph_pattern* pattern =rasqal_query_get_query_graph_pattern(query);
    if (!pattern) {
        BOOST_THROW_EXCEPTION(keto::rdf_utils::InvalidQueryPatternException());
    }

    KETO_LOG_DEBUG << "Get the first trippled sequence";
    raptor_sequence* patternTrippleSequence = rasqal_graph_pattern_get_triples(query,pattern);
    if (!patternTrippleSequence) {
        BOOST_THROW_EXCEPTION(keto::rdf_utils::InvalidQueryPatternException());
    }

    KETO_LOG_DEBUG << "Number of entries is [" << raptor_sequence_size(patternTrippleSequence) << "]";
    if (rasqal_query_get_wildcard(query)) {
        rasqal_query_set_wildcard(query,0);
    }

    KETO_LOG_DEBUG << "The account owner ref";
    if (!containsUri(patternTrippleSequence, Constants::ACCOUNT_OWNER_REF)) {
        KETO_LOG_DEBUG << "Add the owner";
        addTripplePattern(patternTrippleSequence, Constants::ACCOUNT_OWNER_REF,
                Constants::ACCOUNT_OWNER_URI);
    }

    KETO_LOG_DEBUG << "The account group ref";
    if (!containsUri(patternTrippleSequence, Constants::ACCOUNT_GROUP_REF)) {
        KETO_LOG_DEBUG << "Add tripples";
        addTripplePattern(patternTrippleSequence, Constants::ACCOUNT_GROUP_REF,
                Constants::ACCOUNT_GROUP_URI);
    }

    // update the triple information
    rasqal_query_graph_pattern_visit2(this->query,&rasqal_graph_pattern_visit_fn,patternTrippleSequence);

    // check the limit is on the query
    if (rasqal_query_get_limit(this->query) < 0 || rasqal_query_get_limit(this->query)> Constants::SPARQL_MAX_LIMIT) {
        rasqal_query_set_limit(this->query,Constants::SPARQL_DEFAULT_LIMIT);
    }

    KETO_LOG_DEBUG << "The raptor new iostream an store with extr data [" << raptor_sequence_size(patternTrippleSequence) << "]";
    char* updatedQuery;
    size_t querySize;
    raptor_iostream* stream = raptor_new_iostream_to_string(
            rasqal_world_get_raptor(this->world),(void**)&updatedQuery,&querySize,malloc);
    if (rasqal_query_write(stream,query,NULL,NULL)) {
        BOOST_THROW_EXCEPTION(keto::rdf_utils::FailedToProcessQueryException());
    }
    raptor_free_iostream(stream);
    KETO_LOG_DEBUG << "The number of bytes written is : " << querySize;

    if (!updatedQuery) {
        BOOST_THROW_EXCEPTION(keto::rdf_utils::FailedToProcessQueryException());
    }
    KETO_LOG_DEBUG << "Update the query";
    this->sparql = std::string(updatedQuery,querySize);
    KETO_LOG_DEBUG << "After getting they query";
    free(updatedQuery);
}


std::vector<rasqal_literal*> RDFQueryParser::getSubjects(raptor_sequence* patternTrippleSequence) {
    std::vector<rasqal_literal*> subjects;
    std::set<std::string> subjectEntries;
    for (int index = 0; index < raptor_sequence_size(patternTrippleSequence); index++) {
        rasqal_triple *variable = (rasqal_triple *) raptor_sequence_get_at(patternTrippleSequence, index);
        if (variable->subject && variable->subject->type == RASQAL_LITERAL_VARIABLE) {
            if (isExcludedPredicate(variable)) {
                continue;
            }
            std::string subjectName = getVariable(variable->subject);
            if (!subjectEntries.count(subjectName)) {
                subjects.push_back(variable->subject);
                subjectEntries.insert(subjectName);
            }
        }
    }
    return subjects;
}

bool RDFQueryParser::isExcludedPredicate(rasqal_triple* patternTripple) {
    if (!patternTripple->predicate) {
        return false;
    }
    std::string predicate = this->getLiteralInfo(patternTripple->predicate);
    KETO_LOG_DEBUG << "The predicate : " << predicate;
    for (const char* uri : Constants::EXCLUDES) {
        if (predicate.find(uri) == 0) {
            return true;
        }
    }
    return false;
}

std::vector<rasqal_variable*> RDFQueryParser::getVariables(raptor_sequence* patternTrippleSequence) {
    std::vector<rasqal_variable*> variables;
    std::set<std::string> variableEntries;
    for (int index = 0; index < raptor_sequence_size(patternTrippleSequence); index++) {
        rasqal_triple *triple = (rasqal_triple *) raptor_sequence_get_at(patternTrippleSequence, index);
        getVariable(triple->subject, variables, variableEntries);
        getVariable(triple->predicate, variables, variableEntries);
        getVariable(triple->object, variables, variableEntries);
    }
    return variables;
}

void RDFQueryParser::getVariable(rasqal_literal* literal, std::vector<rasqal_variable*>& variables, std::set<std::string>& variableEntries) {
    if (literal && literal->type == RASQAL_LITERAL_VARIABLE) {
        rasqal_variable* variable = literal->value.variable;
        std::string name = getVariable(literal);
        if (!variableEntries.count(name)) {
            variables.push_back(variable);
            variableEntries.insert(name);
        }
    }
}

void RDFQueryParser::addTripplePattern(
        raptor_sequence* patternTrippleSequence, const std::string& predicateUri, const std::string& objectUri) {
        //const std::string& sourceVariableName, const std::string& sourcePredicateUri) {
    std::vector<rasqal_literal*> subjectVar = getSubjects(patternTrippleSequence);
    //if (!subjectVar.size()) {
    //    return;
    //}
    //rasqal_variables_table_add2(subjectVar[0]->value.variable,RASQAL_VARIABLE_TYPE_NORMAL,sourceVariableName.c_str(),
    //                            sourceVariableName.size(),)
    //KETO_LOG_DEBUG << "The source variable" << sourceVariableName;
    //char * strValue = (char *)calloc(sourceVariableName.size() + 1, 0);
    //sprintf(strValue,sourceVariableName.c_str());
    //rasqal_literal* sourceLiteral = rasqal_new_simple_literal(this->world,RASQAL_LITERAL_BLANK,
    //        (const unsigned char *)strValue);

    //rasqal_literal* sourceVariableLiteral = rasqal_new_simple_literal(this->world, RASQAL_LITERAL_VARIABLE,
     //       (const unsigned char *)sourceVariableName.c_str());
    //raptor_sequence_push(patternTrippleSequence,
    //        rasqal_new_triple(
    //                sourceLiteral,
    //                rasqal_new_uri_literal(this->world,
    //                        raptor_new_uri(rasqal_world_get_raptor(this->world), (const unsigned char *)sourcePredicateUri.c_str())),
    //                rasqal_new_simple_literal(this->world, RASQAL_LITERAL_XSD_STRING,
    //                                (const unsigned char *)this->account.c_str())));



    for (rasqal_literal* subject : subjectVar) {
        std::stringstream ss;
        ss << objectUri << this->account;
        //KETO_LOG_DEBUG << "Add the extra tripple to the pattern [" << predicateUri << "][" << sourceVariableLiteral->value.variable->name << "]";
        raptor_sequence_push(patternTrippleSequence, rasqal_new_triple(rasqal_new_literal_from_literal(subject),
                rasqal_new_uri_literal(this->world, raptor_new_uri(
                        rasqal_world_get_raptor(this->world), (const unsigned char *)predicateUri.c_str())),
                rasqal_new_uri_literal(this->world, raptor_new_uri(
                rasqal_world_get_raptor(this->world), (const unsigned char *)ss.str().c_str()))));
    }
}


bool RDFQueryParser::containsUri(raptor_sequence* patternTrippleSequence, const std::string& uri) {
    for (int index = 0; index < raptor_sequence_size(patternTrippleSequence); index++) {
        rasqal_triple* variable = (rasqal_triple*)raptor_sequence_get_at(patternTrippleSequence,index);
        //KETO_LOG_DEBUG << "Tripple";
        //if (variable->subject) {
        //    KETO_LOG_DEBUG << " Subject [" <<  getLiteralInfo(variable->subject) << "]";
        //}
        if (variable->predicate) {
            if (uri == getLiteralInfo(variable->predicate)) {
                return true;
            }
        }
        //if (variable->object) {
        //    KETO_LOG_DEBUG << " Object [" <<  getLiteralInfo(variable->object) << "]";
        //}
        //if (variable->origin) {
        //    KETO_LOG_DEBUG << " Origin [" <<  getLiteralInfo(variable->origin) << "]";
        //}

        //KETO_LOG_DEBUG;
    }
    return false;
}


std::string RDFQueryParser::getLiteralUri(rasqal_literal* variable) {
    std::stringstream ss;
    ss << raptor_uri_as_string(variable->value.uri);
    return ss.str();
}

std::string RDFQueryParser::getLiteralString(rasqal_literal* variable) {
    std::stringstream ss;
    ss << variable->string;
    return ss.str();
}

std::string RDFQueryParser::getLiteralBoolean(rasqal_literal* variable) {
    std::stringstream ss;
    ss << variable->string;
    return ss.str();
}

std::string RDFQueryParser::getLiteralInteger(rasqal_literal* variable) {
    std::stringstream ss;
    ss << variable->value.integer;
    return ss.str();
}

std::string RDFQueryParser::getLiteralFloat(rasqal_literal* variable) {
    std::stringstream ss;
    ss << variable->value.floating;
    return ss.str();
}

std::string RDFQueryParser::getLiteralDecimal(rasqal_literal* variable) {
    std::stringstream ss;
    ss << rasqal_xsd_decimal_as_string(variable->value.decimal);
    return ss.str();
}

std::string RDFQueryParser::getLiteralDateTime(rasqal_literal* variable) {
    std::stringstream ss;
    char * dateTime = rasqal_xsd_datetime_to_string(variable->value.datetime);
    ss << dateTime;
    rasqal_free_memory(dateTime);
    return ss.str();
}

std::string RDFQueryParser::getLiteralDate(rasqal_literal* variable) {
    std::stringstream ss;
    char * date = rasqal_xsd_date_to_string(variable->value.date);
    ss << date;
    rasqal_free_memory(date);
    return ss.str();
}

std::string RDFQueryParser::getVariable(rasqal_literal* variable) {
    std::stringstream ss;
    ss << variable->value.variable->name;
    return ss.str();
}

std::string RDFQueryParser::getLiteralInfo(rasqal_literal* variable) {
    switch(variable->type) {
        case RASQAL_LITERAL_UNKNOWN:
            return "<unknown>";
        case RASQAL_LITERAL_BLANK:
            return "<blank>";
        case RASQAL_LITERAL_URI:
            return getLiteralUri(variable);
        case RASQAL_LITERAL_STRING:
        case RASQAL_LITERAL_XSD_STRING:
            return getLiteralString(variable);
        case RASQAL_LITERAL_BOOLEAN:
            return getLiteralBoolean(variable);
        case RASQAL_LITERAL_INTEGER:
            return getLiteralInteger(variable);
        case RASQAL_LITERAL_FLOAT:
        case RASQAL_LITERAL_DOUBLE:
            return getLiteralFloat(variable);
        case RASQAL_LITERAL_DECIMAL:
            return getLiteralDecimal(variable);
        case RASQAL_LITERAL_DATETIME:
            return getLiteralDateTime(variable);
        case RASQAL_LITERAL_DATE:
            return getLiteralDate(variable);
        case RASQAL_LITERAL_VARIABLE:
            return getVariable(variable);
        default:
            return "<unknown>";
    }
}


}
}
