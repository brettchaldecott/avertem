//
// Created by Brett Chaldecott on 2019/03/04.
//

#include "keto/account_db/AccountGraphDirtySession.hpp"

#include "keto/asn1/Constants.hpp"
#include "keto/account_db/Exception.hpp"

namespace keto {
namespace account_db {

std::string AccountGraphDirtySession::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

AccountGraphDirtySession::AccountGraphDirtySession(const std::string& name) : name(name) {
    this->world = librdf_new_world();
    librdf_world_open(this->world);
    storage=librdf_new_storage(this->world, "memory", NULL, NULL);
    model = librdf_new_model(world,storage,NULL);
}

AccountGraphDirtySession::~AccountGraphDirtySession() {
    librdf_free_model(model);
    librdf_free_storage(storage);
    librdf_free_world(world);
}

std::string AccountGraphDirtySession::getName() {

}

void AccountGraphDirtySession::persistDirty(keto::asn1::RDFSubjectHelperPtr& subject) {
    for (keto::asn1::RDFPredicateHelperPtr predicateHelper : subject->getPredicates()) {
        for (keto::asn1::RDFObjectHelperPtr objectHelper : predicateHelper->listObjects()) {
            librdf_statement* statement= 0;
            if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::LITERAL) == 0) {
                statement = librdf_new_statement_from_nodes(this->world,
                    librdf_new_node_from_uri_string(this->world, (const unsigned char*)subject->getSubject().c_str()),
                    librdf_new_node_from_uri_string(this->world, (const unsigned char*)predicateHelper->getPredicate().c_str()),
                    librdf_new_node_from_typed_literal(
                            this->world,
                            (const unsigned char*)objectHelper->getValue().c_str(),
                            NULL,
                            librdf_new_uri(
                                    this->world,
                                    (const unsigned char*)objectHelper->getDataType().c_str()))
                );
            } else if (objectHelper->getType().compare(keto::asn1::Constants::RDF_NODE::URI) == 0) {
                statement= librdf_new_statement_from_nodes(this->world,
                    librdf_new_node_from_uri_string(this->world, (const unsigned char*)subject->getSubject().c_str()),
                    librdf_new_node_from_uri_string(this->world, (const unsigned char*)predicateHelper->getPredicate().c_str()),
                    librdf_new_node_from_uri_string(this->world, (const unsigned char*)objectHelper->getValue().c_str())
                );
            } else {
                std::stringstream ss;
                ss << "The rdf format [" << objectHelper->getType() << "] is currently not supported";
                BOOST_THROW_EXCEPTION(keto::account_db::UnsupportedDataTypeTransactionException(
                        ss.str()));
            }
            librdf_model_add_statement(this->model, statement);

            /* Free what we just used to add to the model - now it should be stored */
            librdf_free_statement(statement);
        }
    }
}

librdf_model* AccountGraphDirtySession::getDirtyModel() {
    return this->model;
}


}
}
