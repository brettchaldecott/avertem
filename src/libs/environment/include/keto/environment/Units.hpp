//
// Created by Brett Chaldecott on 2019/02/19.
//

#ifndef KETO_UNITS_HPP
#define KETO_UNITS_HPP

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace environment {

class Units {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };


    class TIME {
    public:
        static constexpr const long MILLISECONDS = 1000;
        static constexpr const long NANOSECONDS = 1000000;
        static constexpr const long MICROSECONDS = 1000000000;
    };

};

}
}

#endif //KETO_UNITS_HPP
