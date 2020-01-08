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

    KETO_LOG_ERROR << "[AccountGraphSession::AccountGraphSession] The destructor has been called";
    // free all the extras
    if (world) {
        librdf_free_model(removeModel);
        removeModel = 0;
        librdf_free_model(addModel);
        addModel = 0;
        //librdf_free_model(searchModel);
        //searchModel = 0;
        librdf_free_storage(removeStorage);
        removeStorage = 0;
        librdf_free_storage(addStorage);
        addStorage = 0;
        librdf_free_world(world);
        world = 0;
    }
}

void AccountGraphSession::persistDirty(keto::asn1::RDFSubjectHelperPtr& subject) {
    AccountGraphDirtySessionManager::getInstance()->getDirtySession(this->accountGraphStore->dbName)->persistDirty(subject);
}

void AccountGraphSession::persist(keto::asn1::RDFSubjectHelperPtr& subject) {
    // create a memory model to steam into the disk store
    for (keto::asn1::RDFPredicateHelperPtr predicateHelper : subject->getPredicates()) {
        for (keto::asn1::RDFObjectHelperPtr objectHelper : predicateHelper->listObjects()) {
            librdf_statement* statement= 0;
            if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::LITERAL) == 0) {
                statement = librdf_new_statement_from_nodes(world,
                    librdf_new_node_from_uri_string(world, (const unsigned char*)subject->getSubject().c_str()),
                    librdf_new_node_from_uri_string(world, (const unsigned char*)predicateHelper->getPredicate().c_str()),
                    librdf_new_node_from_typed_literal(
                            world,
                            (const unsigned char*)objectHelper->getValue().c_str(),
                            NULL, 
                            librdf_new_uri(
                                world,
                                (const unsigned char*)objectHelper->getDataType().c_str()))
                    );
            } else if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::URI) == 0) {
                    statement= librdf_new_statement_from_nodes(world,
                        librdf_new_node_from_uri_string(world, (const unsigned char*)subject->getSubject().c_str()),
                        librdf_new_node_from_uri_string(world, (const unsigned char*)predicateHelper->getPredicate().c_str()),
                        librdf_new_node_from_uri_string(world, (const unsigned char*)objectHelper->getValue().c_str())
                        );
            } else {
                std::stringstream ss;
                ss << "The rdf format [" << objectHelper->getType() << "] is currently not supported";
                BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException(
                        ss.str()));
            }
            librdf_model_add_statement(addModel, statement);
             /* Free what we just used to add to the model - now it should be stored */
            librdf_free_statement(statement);
        }
    }
}
    
void AccountGraphSession::remove(keto::asn1::RDFSubjectHelperPtr& subject) {
    for (keto::asn1::RDFPredicateHelperPtr predicateHelper : subject->getPredicates()) {
        for (keto::asn1::RDFObjectHelperPtr objectHelper : predicateHelper->listObjects()) {
            librdf_statement* statement= 0;
            if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::LITERAL) == 0) {
                    statement = librdf_new_statement_from_nodes(world,
                        librdf_new_node_from_uri_string(world, (const unsigned char*)subject->getSubject().c_str()),
                        librdf_new_node_from_uri_string(world, (const unsigned char*)predicateHelper->getPredicate().c_str()),
                        librdf_new_node_from_typed_literal(
                                world,
                                (const unsigned char*)objectHelper->getValue().c_str(),
                                NULL, 
                                librdf_new_uri(
                                    world,
                                    (const unsigned char*)objectHelper->getDataType().c_str()))
                        );
            } else if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::URI) == 0) {
                    statement= librdf_new_statement_from_nodes(world,
                        librdf_new_node_from_uri_string(world, (const unsigned char*)subject->getSubject().c_str()),
                        librdf_new_node_from_uri_string(world, (const unsigned char*)predicateHelper->getPredicate().c_str()),
                        librdf_new_node_from_uri_string(world, (const unsigned char*)objectHelper->getValue().c_str())
                        );
            } else {
                std::stringstream ss;
                ss << "The rdf format [" << objectHelper->getType() << "] is currently not supported";
                BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException(
                        ss.str()));
            }
            librdf_model_add_statement(removeModel, statement);

             /* Free what we just used to add to the model - now it should be stored */
            librdf_free_statement(statement);
        }
    }
}


std::string AccountGraphSession::query(const std::string& queryStr, const std::vector<uint8_t>& accountHash) {
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr,accountHash);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }
    // aquire the read lock
    AccountGraphStore::StorageScopeLockPtr scopeLockPtr = this->accountGraphStore->getStorageLock()->aquireReadLock();

    std::string formatedQuery = rdfQueryParser.getQuery();
    KETO_LOG_ERROR << "[AccountGraphSession::query]Prepare the query [" << formatedQuery << "]";
    librdf_query* query = librdf_new_query(this->accountGraphStore->getWorld(), "sparql",
            NULL, (const unsigned char *)formatedQuery.c_str(), NULL);
    KETO_LOG_ERROR << "[AccountGraphSession::query]Execute the query";
    librdf_query_results* results = librdf_model_query_execute(this->accountGraphStore->getModel(), query);
    if (!results) {
        librdf_free_query(query);
        return "NA";
    }
    KETO_LOG_ERROR << "[AccountGraphSession::query]get the results";
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
    librdf_model_sync(this->accountGraphStore->getModel());
    KETO_LOG_ERROR << "[AccountGraphSession::query]return the results";
    return strResult;
}

ResultVectorMap AccountGraphSession::executeDirtyQuery(const std::string& queryStr, const std::vector<uint8_t>& accountHash) {
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr,accountHash);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }

    // aquire a write lock as the underlying store has to be modified
    AccountGraphStore::StorageScopeLockPtr scopeLockPtr = this->accountGraphStore->getStorageLock()->aquireWriteLock();

    AccountGraphDirtySession::AccountGraphDirtySessionScope accountGraphDirtySessionScope(
            this->accountGraphStore->dbName,this->accountGraphStore->getModel());

    KETO_LOG_DEBUG << "[AccountGraphSession::executeDirtyQuery]Execute the query [" << rdfQueryParser.getQuery() << "]";
    std::string formatedQuery = rdfQueryParser.getQuery();

    librdf_query* query = librdf_new_query(this->accountGraphStore->getWorld(), "sparql",
                             NULL, (const unsigned char *)formatedQuery.c_str(), NULL);

    librdf_query_results* results = librdf_model_query_execute(this->accountGraphStore->getModel(), query);
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
    librdf_model_sync(this->accountGraphStore->getModel());

    KETO_LOG_DEBUG << "Return the results of the query";
    return resultVectorMap;
}


ResultVectorMap AccountGraphSession::executeQuery(const std::string& queryStr, const std::vector<uint8_t>& accountHash) {
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr,accountHash);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }
    KETO_LOG_DEBUG << "[AccountGraphSession::executeQuery]Execute the query [" << rdfQueryParser.getQuery() << "]";
    return this->executeQueryInternal(rdfQueryParser.getQuery());
}

ResultVectorMap AccountGraphSession::executeQueryInternal(const std::string& queryStr) {
    keto::rdf_utils::RDFQueryParser rdfQueryParser(queryStr);
    if (!rdfQueryParser.isValidQuery()) {
        BOOST_THROW_EXCEPTION(keto::account_db::InvalidQueryFormat());
    }

    // aquire the read lock
    AccountGraphStore::StorageScopeLockPtr scopeLockPtr = this->accountGraphStore->getStorageLock()->aquireReadLock();

    std::string formatedQuery = rdfQueryParser.getQuery();
    KETO_LOG_ERROR << "[AccountGraphSession::executeQueryInternal]Prepare the query [" << formatedQuery << "]";
    librdf_query* query = librdf_new_query(this->accountGraphStore->getWorld(), "sparql",
                             NULL, (const unsigned char *)formatedQuery.c_str(), NULL);
    KETO_LOG_ERROR << "[AccountGraphSession::executeQueryInternal]Execute the query";
    librdf_query_results* results = librdf_model_query_execute(this->accountGraphStore->getModel(), query);
    ResultVectorMap resultVectorMap;
    if (!results) {
        librdf_free_query(query);
        return resultVectorMap;
    }
    KETO_LOG_ERROR << "[AccountGraphSession::executeQueryInternal]loop through the results";
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
    librdf_model_sync(this->accountGraphStore->getModel());
    KETO_LOG_ERROR << "[AccountGraphSession::executeQueryInternal]return the results";
    return resultVectorMap;
}

AccountGraphSession::AccountGraphSession(const AccountGraphStorePtr& accountGraphStore) :
    accountGraphStore(accountGraphStore), world(0), addStorage(0), removeStorage(0), addModel(0), removeModel(0) {

    // fall back to using a seperate storage model as the transaction scope for changes
    this->world = librdf_new_world();
    librdf_world_open(this->world);
    this->addStorage=librdf_new_storage(world, "memory", NULL, NULL);
    this->removeStorage=librdf_new_storage(world, "memory", NULL, NULL);
    //this->searchModel = librdf_new_model(this->accountGraphStore->getWorld(),this->accountGraphStore->getStorage(),NULL);
    this->addModel = librdf_new_model(world,addStorage,NULL);
    this->removeModel = librdf_new_model(world,removeStorage,NULL);

}

void AccountGraphSession::commit() {
    KETO_LOG_ERROR << "[AccountGraphSession::commit] commit the changes [" << librdf_model_size(addModel) << "][" << librdf_model_size(removeModel) << "]";
    if (librdf_model_size(addModel) || librdf_model_size(removeModel)) {

        // aquire the write lock
        AccountGraphStore::StorageScopeLockPtr scopeLockPtr = this->accountGraphStore->getStorageLock()->aquireWriteLock();

        // only commit if there are changes in the model
        if (librdf_model_size(addModel)) {
            // now we stream the memory into the db object
            KETO_LOG_ERROR << "[AccountGraphSession::commit] changes are being streamed to model";
            librdf_stream *stream = librdf_model_as_stream(this->addModel);
            librdf_model_add_statements(this->accountGraphStore->getModel(), stream);
            librdf_free_stream(stream);
            // sync the changes to the store otherwise we have to wait for close
            librdf_model_sync(this->accountGraphStore->getModel());
            KETO_LOG_ERROR << "[AccountGraphSession::commit] changes have been applied";
        }
        if (librdf_model_size(removeModel)) {
            KETO_LOG_ERROR << "[AccountGraphSession::commit] changes are being removed";
            // now we stream into the memory object and remove statement by statement
            librdf_stream *stream = librdf_model_as_stream(this->addModel);
            while (!librdf_stream_end(stream)) {
                librdf_model_remove_statement(this->accountGraphStore->getModel(), librdf_stream_get_object(stream));
                librdf_stream_next(stream);
            }
            librdf_free_stream(stream);
            // sync the changes to the store otherwise we have to wait for close
            librdf_model_sync(this->accountGraphStore->getModel());
            KETO_LOG_ERROR << "[AccountGraphSession::commit] changes are been removed";
        }
    }
}

void AccountGraphSession::rollback() {

}


}
}
