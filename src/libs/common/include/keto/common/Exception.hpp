/* 
 * File:   Exception.hpp
 * Author: Brett Chaldecott
 *
 * Created on January 11, 2018, 11:08 AM
 */

#ifndef KETO_EXCEPTION_HPP
#define KETO_EXCEPTION_HPP

#include <stdio.h>
#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <exception>

namespace keto {
namespace common {

class Exception : public std::exception, public boost::exception {
public:
    Exception() noexcept;
    Exception(const std::string& msg) noexcept;
    Exception(const std::string& type, const std::string& msg) noexcept;
    Exception(const Exception& orig) noexcept;
    virtual ~Exception();
    Exception& operator= (const Exception& orig) noexcept;

    std::string getType() const noexcept;
    virtual const char* what() const noexcept;

private:
    std::string type;
    std::string msg;

};

#define TEXTIFY(A) #A

// This mack
#define KETO_DECLARE_EXCEPTION( TYPE, WHAT ) \
   class TYPE : public keto::common::Exception \
   { \
      public: \
       TYPE() noexcept : Exception(TEXTIFY(TYPE),WHAT) {} \
       TYPE( const std::string& msg) noexcept : Exception(TEXTIFY(TYPE),msg) {} \
       TYPE( const std::string& type, const std::string& msg) noexcept : Exception(type,msg) {} \
       TYPE( const TYPE& c ) : Exception(c) {} \
   };

#define KETO_DECLARE_DERIVED_EXCEPTION( BASE, TYPE, WHAT ) \
   class TYPE : public BASE \
   { \
      public: \
       TYPE() noexcept : BASE(TEXTIFY(TYPE),WHAT) {} \
       TYPE( const std::string& msg) noexcept : BASE(TEXTIFY(TYPE),msg) {} \
       TYPE( const std::string& type, const std::string& msg) noexcept : BASE(type,msg) {} \
       TYPE( const TYPE& c ) : BASE(c) {} \
   };

}
}

#endif /* EXCEPTION_HPP */

