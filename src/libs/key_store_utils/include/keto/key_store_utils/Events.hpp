/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Events.hpp
 * Author: brettchaldecott
 *
 * Created on 17 November 2018, 11:04 AM
 */

#ifndef KETO_STORE_UTILS_EVENTS_HPP
#define KETO_STORE_UTILS_EVENTS_HPP

namespace keto {
namespace key_store_utils {

class Events {
public:
    Events() = delete;
    Events(const Events& orig) = delete;
    virtual ~Events() = delete;
    
    class TRANSACTION {
    public:
        static const char* REENCRYPT_TRANSACTION;
        static const char* ENCRYPT_TRANSACTION;
        static const char* DECRYPT_TRANSACTION;
    };
    
private:

};


}
}


#endif /* EVENTS_HPP */

