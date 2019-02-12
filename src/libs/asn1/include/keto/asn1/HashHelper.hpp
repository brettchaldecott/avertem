/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HashHelper.hpp
 * Author: ubuntu
 *
 * Created on February 2, 2018, 4:03 AM
 */

#ifndef HASHHELPER_HPP
#define HASHHELPER_HPP

#include <string>
#include <algorithm>

#include "Hash.h"
#include "keto/common/StringCodec.hpp"

#include "keto/crypto/Containers.hpp"

#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace asn1 {

class HashHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    HashHelper();
    HashHelper(const std::string& hash);
    HashHelper(const std::vector<uint8_t>& hash);
    HashHelper(const keto::crypto::SecureVector& hash);
    HashHelper(const Hash_t& hash);
    HashHelper(const std::string& hash,keto::common::StringEncoding stringEncoding);
    
    HashHelper(const HashHelper& orig) = default;
    virtual ~HashHelper();
    
    HashHelper& operator=(const std::string& hash);
    
    HashHelper& operator=(const Hash_t* hash);
    HashHelper& operator=(const Hash_t& hash);
    operator Hash_t() const;
    
    HashHelper& operator =(const keto::crypto::SecureVector& hash);
    operator keto::crypto::SecureVector() const;
    operator std::vector<uint8_t>() const;
    operator std::string() const;

     
    HashHelper& setHash(const std::string& hash,keto::common::StringEncoding stringEncoding);
    std::string getHash(keto::common::StringEncoding stringEncoding) const;
    
    bool empty();
    
private:
    keto::crypto::SecureVector hash;
    
    
    void copyHashToVector(const Hash_t& hash, keto::crypto::SecureVector& vector);
};

bool operator==(const HashHelper& lhs, const std::vector<uint8_t>& rhs) {
    return (std::vector<uint8_t >)lhs == rhs;
};

bool operator==(const HashHelper& lhs, const HashHelper& rhs) {
    return (std::vector<uint8_t >)lhs == (std::vector<uint8_t >)rhs;
};

}
}


#endif /* HASHHELPER_HPP */

