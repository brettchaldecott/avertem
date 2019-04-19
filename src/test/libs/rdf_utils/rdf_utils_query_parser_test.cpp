/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#define BOOST_TEST_MODULE ChainCommonsTest

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <botan/hash.h>
#include <botan/rsa.h>
#include <botan/rng.h>
#include <botan/p11_randomgenerator.h>
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <botan/x509_key.h>
#include "TestEntity.h"
#include "Number.h"
#include "keto/common/MetaInfo.hpp"
#include "keto/asn1/TimeHelper.hpp"
#include "keto/asn1/SerializationHelper.hpp"
#include "keto/asn1/DeserializationHelper.hpp"

#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"
#include "keto/chain_common/ActionBuilder.hpp"
#include "keto/crypto/SignatureVerification.hpp"
#include "keto/rdf_utils/RDFQueryParser.hpp"

BOOST_AUTO_TEST_CASE( rdf_utils_query_parser_test ) {
    const char* query = R"(SELECT * WHERE { ?a ?b ?c . } )";
    keto::rdf_utils::RDFQueryParser rdfQueryParser(query);
    BOOST_CHECK( rdfQueryParser.isValidQuery() );

    std::cout<< "Query : " << rdfQueryParser.getQuery() << std::endl;

    keto::rdf_utils::RDFQueryParser rdfQueryParser2(query,"account_value");
    BOOST_CHECK( rdfQueryParser2.isValidQuery() );
    std::cout<< "Query : " << rdfQueryParser2.getQuery() << std::endl;

    const char* query2 = R"(SELECT ?id ?blockId ?date ?account ?accountHash ?type ?name ?value WHERE {
?transaction <http://keto-coin.io/schema/rdf/1.0/keto/Transaction#id> ?id .
?transaction <http://keto-coin.io/schema/rdf/1.0/keto/Transaction#block> ?block .
?block <http://keto-coin.io/schema/rdf/1.0/keto/Block#id> ?blockId .
?transaction <http://keto-coin.io/schema/rdf/1.0/keto/Transaction#date> ?date .
?transaction <http://keto-coin.io/schema/rdf/1.0/keto/Transaction#account>  "test_value"^^<http://www.w3.org/2001/XMLSchema#string> .
?transaction <http://keto-coin.io/schema/rdf/1.0/keto/Transaction#account> ?account .
?accountTransaction <http://keto-coin.io/schema/rdf/1.0/keto/Account#transaction> ?transaction .
?accountTransaction <http://keto-coin.io/schema/rdf/1.0/keto/AccountTransaction#type> ?type .
?accountTransaction <http://keto-coin.io/schema/rdf/1.0/keto/AccountTransaction#name> ?name .
?accountTransaction <http://keto-coin.io/schema/rdf/1.0/keto/AccountTransaction#accountHash> ?accountHash .
?accountTransaction <http://keto-coin.io/schema/rdf/1.0/keto/AccountTransaction#value> ?value .
} ORDER BY DESC(?date) LIMIT 10 )";

    keto::rdf_utils::RDFQueryParser rdfQueryParser3(query2,"account_value");
    BOOST_CHECK( rdfQueryParser3.isValidQuery() );
    std::cout<< "Query : " << rdfQueryParser3.getQuery() << std::endl;
}