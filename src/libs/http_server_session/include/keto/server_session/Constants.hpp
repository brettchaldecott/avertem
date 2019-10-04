//
// Created by Brett Chaldecott on 2019/03/07.
//

#ifndef KETO_SERVER_SESSION_CONSTANTS_HPP
#define KETO_SERVER_SESSION_CONSTANTS_HPP

namespace keto {
namespace server_session {


class Constants {
public:
    static const char* HTTP_SESSION_ACCOUNT;

    class CONTENT_TYPE {
    public:
        static const char* HTML;
        static const char* TEXT;
        static const char* JSON;
        static const char* SPARQL;
        static const char* PROTOBUF;
    };
};


}
}


#endif //KETO_CONSTANTS_HPP
