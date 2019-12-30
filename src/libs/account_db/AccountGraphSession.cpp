/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AccountGraphSession.cpp
 * Author: ubuntu
 * 
 * Created on March 28, 2018, 6:00 AM
 */

#include <iostream>
#include <sstream>

#include "keto/asn1/Constants.hpp"
#include "keto/account_db/AccountGraphSession.hpp"
#include "keto/account_db/Exception.hpp"
#include "keto/account_db/AccountGraphStore.hpp"
#include "keto/account_db/AccountGraphSession.hpp"
#include "keto/account_db/AccountGraphDirtySessionManager.hpp"
#include "keto/common/Log.hpp"
#include "keto/rdf_utils/RDFQueryParser.hpp"

namespace keto {
namespace account_db {

    
// TODO: Implement a custom Berkely DB loader to support full transactions
// at present transaction rollback is not supported.
std::string AccountGraphSession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
    
AccountGraphSession::~AccountGraphSession() {
    if (activeTransaction) {
        // rollback if active still at this point
        this->rollback();
    }
}

void AccountGraphSession::persistDirty(keto::asn1::RDFSubjectHelperPtr& subject) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    AccountGraphDirtySessionManager::getInstance()->getDirtySession(this->accountGraphStore->dbName)->persistDirty(subject);
}

void AccountGraphSession::persist(keto::asn1::RDFSubjectHelperPtr& subject) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    for (keto::asn1::RDFPredicateHelperPtr predicateHelper : subject->getPredicates()) {
        for (keto::asn1::RDFObjectHelperPtr objectHelper : predicateHelper->listObjects()) {
            librdf_statement* statement= 0;
            if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::LITERAL) == 0) {
                statement = librdf_new_statement_from_nodes(this->accountGraphStore->getWorld(), 
                    librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)subject->getSubject().c_str()),
                    librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)predicateHelper->getPredicate().c_str()),
                    librdf_new_node_from_typed_literal(
                            this->accountGraphStore->getWorld(), 
                            (const unsigned char*)objectHelper->getValue().c_str(),
                            NULL, 
                            librdf_new_uri(
                                this->accountGraphStore->getWorld(),
                                (const unsigned char*)objectHelper->getDataType().c_str()))
                    );
            } else if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::URI) == 0) {
                    statement= librdf_new_statement_from_nodes(this->accountGraphStore->getWorld(), 
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)subject->getSubject().c_str()),
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)predicateHelper->getPredicate().c_str()),
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)objectHelper->getValue().c_str())
                        );
            } else {
                std::stringstream ss;
                ss << "The rdf format [" << objectHelper->getType() << "] is currently not supported";
                BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException(
                        ss.str()));
            }
            librdf_model_add_statement(this->accountGraphStore->getModel(), statement);

             /* Free what we just used to add to the model - now it should be stored */
            librdf_free_statement(statement);
        }
    }
}
    
void AccountGraphSession::remove(keto::asn1::RDFSubjectHelperPtr& subject) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    for (keto::asn1::RDFPredicateHelperPtr predicateHelper : subject->getPredicates()) {
        for (keto::asn1::RDFObjectHelperPtr objectHelper : predicateHelper->listObjects()) {
            librdf_statement* statement= 0;
            if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::LITERAL) == 0) {
                    statement = librdf_new_statement_from_nodes(this->accountGraphStore->getWorld(), 
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)subject->getSubject().c_str()),
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)predicateHelper->getPredicate().c_str()),
                        librdf_new_node_from_typed_literal(
                                this->accountGraphStore->getWorld(), 
                                (const unsigned char*)objectHelper->getValue().c_str(),
                                NULL, 
                                librdf_new_uri(
                                    this->accountGraphStore->getWorld(),
                                    (const unsigned char*)objectHelper->getDataType().c_str()))
                        );
            } else if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::URI) == 0) {
                    statement= librdf_new_statement_from_nodes(this->accountGraphStore->getWorld(), 
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)subject->getSubject().c_str()),
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)predicateHelper->getPredicate().c_str()),
                        librdf_new_node_from_uri_string(this->accountGraphStore->getWorld(), (const unsigned char*)objectHelper->getValue().c_str())
                        );
            } else {
                std::stringstream ss;
                ss << "The rdf format [" << objectHelper->getType() << "] is currently not supported";
                BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException(
                        ss.str()));
            }
            librdf_model_remove_statement(this->accountGraphStore->getModel(), statement);

             /* Free what we just used to add to the model - now it should be stored */
            librdf_free_statement(statement);
        }
    }
}


std::string AccountGraphSession::query(const std::string& queryStr, const std::vector<uint8_t>& accountHash) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr,accountHash);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }
    librdf_query* query;
    librdf_query_results* results;
    std::string formatedQuery = rdfQueryParser.getQuery();
    query = librdf_new_query(this->accountGraphStore->getWorld(), "sparql",
            NULL, (const unsigned char *)formatedQuery.c_str(), NULL);
    results = librdf_model_query_execute(this->accountGraphStore->getModel(), query);
    if (!results) {
        librdf_free_query(query);
        return "NA";
    }
    unsigned char* strBuffer = librdf_query_results_to_string2(results,"json",NULL,NULL,NULL);
    if (!strBuffer) {
        librdf_free_query_results(results);
        librdf_free_query(query);
        return "NA";
    }
    std::string strResult((const char*)strBuffer);
    
    librdf_free_memory(strBuffer);
    librdf_free_query_results(results);
    librdf_free_query(query);
    
    return strResult;
}

ResultVectorMap AccountGraphSession::executeDirtyQuery(const std::string& queryStr, const std::vector<uint8_t>& accountHash) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    //if (librdf_model_add_submodel(this->accountGraphStore->getModel(),
    //        AccountGraphDirtySessionManager::getInstance()->getDirtySession(this->accountGraphStore->dbName)->getDirtyModel())) {
    //    KETO_LOG_DEBUG << "Faild to add the sub models";
    //    BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException());
    //}
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr,accountHash);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }
    AccountGraphDirtySession::AccountGraphDirtySessionScope accountGraphDirtySessionScope(
            this->accountGraphStore->dbName,this->accountGraphStore->getModel());
    KETO_LOG_DEBUG << "[AccountGraphSession::executeDirtyQuery]Execute the query [" << rdfQueryParser.getQuery() << "]";
    std::string formatedQuery = rdfQueryParser.getQuery();
    librdf_query* query;
    librdf_query_results* results;
    query = librdf_new_query(this->accountGraphStore->getWorld(), "sparql",
                             NULL, (const unsigned char *)formatedQuery.c_str(), NULL);

    results = librdf_model_query_execute(this->accountGraphStore->getModel(), query);
    ResultVectorMap resultVectorMap;
    if (!results) {
        KETO_LOG_DEBUG << "[AccountGraphSession::executeDirtyQuery]Return the empty results";
        librdf_free_query(query);
        return resultVectorMap;
    }

    while (!librdf_query_results_finished(results)) {
        ResultMap resultMap;
        const char **names = NULL;
        librdf_node *nodes[librdf_query_results_get_bindings_count(results)];

        if (librdf_query_results_get_bindings(results, &names, nodes)) {
            KETO_LOG_DEBUG << "[AccountGraphSession::executeDirtyQuery]Break from the loop as no results where found";
            break;
        }
        if (names) {
            for (int index = 0; names[index]; index++) {

                unsigned char *value = librdf_node_get_literal_value(nodes[index]);
                if (value) {
                    resultMap[names[index]] = std::string((const char *) value);
                } else {
                    resultMap[names[index]] = "";
                }
                librdf_free_node(nodes[index]);
            }
        }
        resultVectorMap.push_back(resultMap);
        librdf_query_results_next(results);
    }


    librdf_free_query_results(results);
    librdf_free_query(query);
    //if (librdf_model_remove_submodel(this->accountGraphStore->getModel(),
    //                          AccountGraphDirtySessionManager::getInstance()->getDirtySession(this->accountGraphStore->dbName)->getDirtyModel())) {
    //    KETO_LOG_DEBUG << "Failed to remove the sub model";
    //    BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException());
    //}

    KETO_LOG_DEBUG << "Return the results of the query";
    return resultVectorMap;
}


ResultVectorMap AccountGraphSession::executeQuery(const std::string& queryStr, const std::vector<uint8_t>& accountHash) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr,accountHash);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }
    KETO_LOG_DEBUG << "[AccountGraphSession::executeQuery]Execute the query [" << rdfQueryParser.getQuery() << "]";
    return this->executeQueryInternal(rdfQueryParser.getQuery());
}

ResultVectorMap AccountGraphSession::executeQueryInternal(const std::string& queryStr) {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }

    std::string formatedQuery = rdfQueryParser.getQuery();
    librdf_query* query;
    librdf_query_results* results;
    query = librdf_new_query(this->accountGraphStore->getWorld(), "sparql",
                             NULL, (const unsigned char *)formatedQuery.c_str(), NULL);
    results = librdf_model_query_execute(this->accountGraphStore->getModel(), query);
    ResultVectorMap resultVectorMap;
    if (!results) {
        librdf_free_query(query);
        return resultVectorMap;
    }
    while (!librdf_query_results_finished(results)) {
        ResultMap resultMap;
        const char **names=NULL;
        librdf_node* nodes[librdf_query_results_get_bindings_count(results)];

        if(librdf_query_results_get_bindings(results, &names, nodes))
            break;
        if (names) {
            for (int index=0; names[index]; index++) {
                unsigned char *value = librdf_node_get_literal_value(nodes[index]);
                if (value) {
                    resultMap[names[index]] = std::string((const char *) value);
                } else {
                    resultMap[names[index]] = "";
                }
                librdf_free_node(nodes[index]);
            }
        }
        resultVectorMap.push_back(resultMap);
        librdf_query_results_next(results);
    }
    librdf_free_query_results(results);
    librdf_free_query(query);
    return resultVectorMap;
}

AccountGraphSession::AccountGraphSession(const AccountGraphStorePtr& accountGraphStore) :
    activeTransaction(true), accountGraphStore(accountGraphStore) {
    
    if (librdf_model_transaction_start(this->accountGraphStore->getModel())) {
        // the current back end store does not support transactions
        this->activeTransaction = false;
        KETO_LOG_DEBUG << "Bankend store for this model does not support transactions : "  << this->accountGraphStore->getDbName();
    }
}

void AccountGraphSession::commit() {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    if (this->activeTransaction) {
        if (librdf_model_transaction_commit(this->accountGraphStore->getModel()) ) {
            std::stringstream ss;
            ss << "Failed to commit graph transaction for [" << accountGraphStore->getDbName() << "]";
            BOOST_THROW_EXCEPTION(keto::account_db::FailedToCommitGraphTransactionException(
                    ss.str()));
        }
        this->activeTransaction = false;
    } else {
        // sync the changes to the store otherwise we have to wait for close
        librdf_model_sync(this->accountGraphStore->getModel());
    }
}

void AccountGraphSession::rollback() {
    std::lock_guard<std::recursive_mutex> guard(classMutex);
    if (this->activeTransaction) {
        if (librdf_model_transaction_rollback(this->accountGraphStore->getModel()) ) {
            std::stringstream ss;
            ss << "Failed to rollback graph transaction for [" << accountGraphStore->getDbName() << "]";
            BOOST_THROW_EXCEPTION(keto::account_db::FailedToRollbackGraphTransactionException(
                    ss.str()));
        }
        this->activeTransaction = false;
    } else {
        // sync the changes to the store otherwise we have to wait for close
        // this is required
        librdf_model_sync(this->accountGraphStore->getModel());
    }
}


}
}
