/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Events.cpp
 * Author: brettchaldecott
 * 
 * Created on 17 November 2018, 11:04 AM
 */

#include "keto/key_store_utils/Events.hpp"

namespace keto {
namespace key_store_utils {

const char* Events::TRANSACTION::REENCRYPT_TRANSACTION = "REENCRYPT_TRANSACTION";
const char* Events::TRANSACTION::ENCRYPT_TRANSACTION = "ENCRYPT_TRANSACTION";
const char* Events::TRANSACTION::DECRYPT_TRANSACTION = "DECRYPT_TRANSACTION";

}
}
