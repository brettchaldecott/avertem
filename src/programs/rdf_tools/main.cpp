
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

#include <botan/hex.h>
#include <botan/base64.h>
#include <botan/ecdsa.h>

#include <keto/server_common/StringUtils.hpp>
#include <rasqal/rasqal.h>


#include "keto/common/MetaInfo.hpp"
#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"
#include "keto/ssl/RootCertificate.hpp"
#include "keto/session/HttpSession.hpp"
#include "keto/session/HttpSessionTransactionEncryptor.hpp"

#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"
#include "keto/chain_common/ActionBuilder.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/TransactionTraceBuilder.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/crypto/HashGenerator.hpp"

#include "keto/rdf_tools/Constants.hpp"
#include "rasqal.h"
#include "raptor.h"


namespace ketoEnv = keto::environment;
namespace ketoCommon = keto::common;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

boost::program_options::options_description generateOptionDescriptions() {
    boost::program_options::options_description optionDescripion;
    
    optionDescripion.add_options()
            ("help,h", "Print this help message and exit.")
            ("version,v", "Print version information.")
            ("validate,V", "Validate query.")
            ("sparql,s", po::value<std::string>(),"Sparql rdf.");

    
    return optionDescripion;
}

std::string getLiteralUri(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "URI : " << raptor_uri_as_string(variable->value.uri);
    return ss.str();
}

std::string getLiteralString(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "String : " << variable->string;
    return ss.str();
}

std::string getLiteralBoolean(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "Boolean : " << variable->string;
    return ss.str();
}

std::string getLiteralInteger(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "Boolean : " << variable->value.integer;
    return ss.str();
}

std::string getLiteralFloat(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "Float : " << variable->value.floating;
    return ss.str();
}

std::string getLiteralDecimal(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "Decimal : " << rasqal_xsd_decimal_as_string(variable->value.decimal);
    return ss.str();
}

std::string getLiteralDateTime(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "DateTime : " << rasqal_xsd_datetime_to_string(variable->value.datetime);
    return ss.str();
}

std::string getLiteralDate(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "Date : " << rasqal_xsd_date_to_string(variable->value.date);
    return ss.str();
}

std::string getVariable(rasqal_literal* variable) {
    std::stringstream ss;
    ss << "Variable : " << variable->value.variable->name;
    return ss.str();
}

std::string getLiteralInfo(rasqal_literal* variable) {
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

int validateQuery(std::shared_ptr<ketoEnv::Config> config,
                        boost::program_options::options_description optionDescription) {
    std::string sparql;
    if (config->getVariablesMap().count(keto::rdf_tools::Constants::SPARQL)) {
        sparql = config->getVariablesMap()[keto::rdf_tools::Constants::SPARQL].as<std::string>();
    } else {
        std::cout << "Must provide the sparql query" << std::endl;
        return -1;
    }

    rasqal_world *world;
    world=rasqal_new_world();
    if(!world || rasqal_world_open(world)) {
        std::cout <<" Failed to init the world" << std::endl;
        return(1);
    }

    rasqal_query *query = rasqal_new_query(world, keto::rdf_tools::Constants::SPARQL, NULL);
    if (rasqal_query_prepare(query, (const unsigned char *)sparql.c_str(), NULL)) {
        std::cout <<"The query is invalid" << std::endl;
        return(1);
    }

    if (rasqal_query_get_verb(query) != RASQAL_QUERY_VERB_SELECT) {
        std::cout <<"Only selects are supported" << std::endl;
        return(1);
    }

    raptor_sequence* variableSequence = rasqal_query_get_all_variable_sequence(query);
    //raptor_sequence* variableSequence = rasqal_query_get_bound_variable_sequence(query);
    if (!variableSequence) {
        std::cout <<"variable sequence is invalid" << std::endl;
        return(1);
    }

    for (int index = 0; index < raptor_sequence_size(variableSequence); index++) {
        rasqal_variable* variable = (rasqal_variable*)raptor_sequence_get_at(variableSequence,index);

        if (variable->expression != NULL) {
            std::cout << "[" << variable->name << "]["
                      << variable->expression->name << "]" << std::endl;
        } else {
            std::cout << "[" << variable->name << "][NONE]" << std::endl;
        }
    }

    rasqal_graph_pattern* pattern =rasqal_query_get_query_graph_pattern(query);
    if (!pattern) {
        std::cout <<"Pattern sequence is invalid" << std::endl;
        //    return(1);
    }
    std::cout << "The pattern is : " << rasqal_graph_pattern_operator_as_string(rasqal_graph_pattern_get_operator(pattern)) << std::endl;
    raptor_sequence* patternTrippleSequence = rasqal_graph_pattern_get_triples(query,pattern);
    if (!patternTrippleSequence) {
        std::cout <<"Tripple sequence is invalid" << std::endl;
        return(1);
    }

    for (int index = 0; index < raptor_sequence_size(patternTrippleSequence); index++) {
        rasqal_triple* variable = (rasqal_triple*)raptor_sequence_get_at(patternTrippleSequence,index);
        std::cout << "Tripple";
        if (variable->subject) {
            std::cout << " Subject [" <<  getLiteralInfo(variable->subject) << "]";
        }
        if (variable->predicate) {
            std::cout << " Predicate [" <<  getLiteralInfo(variable->predicate) << "]";
        }
        if (variable->object) {
            std::cout << " Object [" <<  getLiteralInfo(variable->object) << "]";
        }
        if (variable->origin) {
            std::cout << " Origin [" <<  getLiteralInfo(variable->origin) << "]";
        }

        std::cout << std::endl;
    }

    //raptor_sequence* patternSequence = rasqal_query_get_graph_pattern_sequence(query);
    //raptor_sequence* variableSequence = rasqal_query_get_bound_variable_sequence(query);
    //if (!patternSequence) {
    //    std::cout <<"Pattern sequence is invalid" << std::endl;
    //    return(1);
    //}

    //for (int index = 0; index < raptor_sequence_size(patternSequence); index++) {
    //    rasqal_graph_pattern* pattern = (rasqal_graph_pattern*)raptor_sequence_get_at(patternSequence,index);

    //    std::cout << "The pattern is : " << rasqal_graph_pattern_operator_as_string(rasqal_graph_pattern_get_operator(pattern)) << std::endl;


    //}

    raptor_sequence* dataGraph = rasqal_query_get_data_graph_sequence(query);
    if (!dataGraph) {
        std::cout <<"variable sequence is invalid" << std::endl;
        return(1);
    }
    for (int index = 0; index < raptor_sequence_size(dataGraph); index++) {
        rasqal_data_graph* variable = rasqal_query_get_data_graph(query,index);
        //if (variable-> != NULL) {
        //    std::cout << "[" << variable->name << "]["
        //              << variable->expression->name << "]" << std::endl;
        //} else {
            std::cout << "[" << variable->format_name << "][" << variable->format_type << "]" << std::endl;
        //}
    }

    raptor_sequence* tippleSeq = rasqal_query_get_triple_sequence(query);
    if (!tippleSeq) {
        std::cout <<"variable sequence is invalid" << std::endl;
        return(1);
    }
    for (int index = 0; index < raptor_sequence_size(tippleSeq); index++) {
        rasqal_triple* variable = rasqal_query_get_triple(query,index);
        //if (variable-> != NULL) {
        //    std::cout << "[" << variable->name << "]["
        //              << variable->expression->name << "]" << std::endl;
        //} else {
        std::cout << "[" << variable->subject->string << "]" << std::endl;
        //}
    }



    /*raptor_sequence* expressionSequence = rasqal_query_get_order_conditions_sequence(query)
    if (!expressionSequence) {
        std::cout <<"expression sequence is invalid" << std::endl;
        return(1);
    }
    for (int index = 0; index < raptor_sequence_size(expressionSequence); index++) {
        rasqal_expression* expression = rasqal_query_get_variable(query,index);
        std::cout << "Variable name is : " << variable->name << std::endl;
    }*/
    return 0;
}


/**
 * The CLI main file
 * 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char** argv)
{
    try {
        boost::program_options::options_description optionDescription =
                generateOptionDescriptions();
        
        std::shared_ptr<ketoEnv::EnvironmentManager> manager = 
                keto::environment::EnvironmentManager::init(
                keto::environment::Constants::KETO_CLI_CONFIG_FILE,
                optionDescription,argc,argv);
        
        std::shared_ptr<ketoEnv::Config> config = manager->getConfig();
        
        if (config->getVariablesMap().count(ketoEnv::Constants::KETO_VERSION)) {
            std::cout << ketoCommon::MetaInfo::VERSION << std::endl;
            return 0;
        }
        
        if (config->getVariablesMap().count(ketoEnv::Constants::KETO_HELP)) {
            std::cout << "Example:" << std::endl;
            std::cout << "\t./rdf_tool.sh -V -s \"query\"" << std::endl;
            std::cout <<  optionDescription << std::endl;
            return 0;
        }
        
        if (config->getVariablesMap().count(keto::rdf_tools::Constants::VALIDATE)) {
            return validateQuery(config,optionDescription);
        }
        KETO_LOG_INFO << "CLI Executed";
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "Failed to start because : " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
        return -1;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "Failed to start because : " << boost::diagnostic_information(ex,true);
        return -1;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "Failed to start because : " << ex.what();
        return -1;
    } catch (...) {
        KETO_LOG_ERROR << "Failed to start unknown error.";
        return -1;
    }
}
