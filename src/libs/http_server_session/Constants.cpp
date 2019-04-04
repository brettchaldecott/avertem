//
// Created by Brett Chaldecott on 2019/03/07.
//

#include "keto/server_session/Constants.hpp"


namespace keto{
namespace server_session {

const char* Constants::HTTP_SESSION_ACCOUNT = "HTTP_SESSION_ACCOUNT";

const char* Constants::CONTENT_TYPE::HTML   = "text/html";
const char* Constants::CONTENT_TYPE::TEXT   = "text/plain";
const char* Constants::CONTENT_TYPE::JSON   = "application/json";
const char* Constants::CONTENT_TYPE::SPARQL = "application/sparql-results+json";

}
}