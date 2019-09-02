/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EventUtils.hpp
 * Author: ubuntu
 *
 * Created on February 17, 2018, 10:33 AM
 */

#ifndef KETO_EVENTUTILS_HPP
#define KETO_EVENTUTILS_HPP

#include <string>
#include <vector>

#include "keto/server_common/VectorUtils.hpp"
#include "keto/event/Event.hpp"
#include "keto/server_common/Exception.hpp"
#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace server_common {

class EventUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
};

    
template<class PbType>
keto::event::Event toEvent(const PbType& type) {
    std::string pbValue;
    if (!type.SerializeToString(&pbValue)) {
        BOOST_THROW_EXCEPTION(keto::server_common::ProtobuffSerializationException());
    }
    return keto::event::Event(VectorUtils().copyStringToVector(pbValue));
}
    
template<class PbType>
keto::event::Event toEvent(const std::string& event, const PbType& type) {
    std::string pbValue;
    if (!type.SerializeToString(&pbValue)) {
        BOOST_THROW_EXCEPTION(keto::server_common::ProtobuffSerializationException());
    }
    return keto::event::Event(event,VectorUtils().copyStringToVector(pbValue));
}

template<class PbType>
PbType fromEvent(const keto::event::Event& event) {
    PbType pbValue;
    keto::event::Event copyEvent = event;
    if (!pbValue.ParseFromString(VectorUtils().copyVectorToString(copyEvent.getMessage()))) {
        BOOST_THROW_EXCEPTION(keto::server_common::ProtobuffDeserializationException());
    }
    return pbValue;
}
    
}
}


#endif /* EVENTUTILS_HPP */

