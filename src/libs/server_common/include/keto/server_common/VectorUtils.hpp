/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VectorUtils.hpp
 * Author: ubuntu
 *
 * Created on February 16, 2018, 11:21 AM
 */

#ifndef SERVER_COMMON_VECTORUTILS_HPP
#define SERVER_COMMON_VECTORUTILS_HPP

#include <vector>
#include <string>

#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace server_common {


class VectorUtils {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    static std::string getSourceVersion();

    VectorUtils();
    VectorUtils(const VectorUtils& orig) = default;
    virtual ~VectorUtils();
    
    /**
     * This copies from a string that is normally supplied by protobuf to a
     * binary vector. This handles the type conversion.
     * 
     * @param str The string to copy.
     * @return The resulting vector.
     */
    std::vector<uint8_t> copyStringToVector(const std::string& str);
    
    
    /**
     * This method copies a vector to a string using a string stream.
     * 
     * @param vec The vector to copy.
     * @return The string containing a copy of the vector
     */
    std::string copyVectorToString(const std::vector<uint8_t>& vec);
private:

};


}
}

#endif /* VECTORUTILS_HPP */

