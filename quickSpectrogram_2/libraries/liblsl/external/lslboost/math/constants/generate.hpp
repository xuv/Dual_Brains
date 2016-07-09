//  Copyright John Maddock 2010.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.lslboost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_CONSTANTS_GENERATE_INCLUDED
#define BOOST_MATH_CONSTANTS_GENERATE_INCLUDED

#include <lslboost/math/constants/constants.hpp>
#include <lslboost/regex.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef USE_MPFR
#include <lslboost/math/bindings/mpfr.hpp>
#elif defined(USE_MPREAL)
#include <lslboost/math/bindings/mpreal.hpp>
#elif defined(USE_CPP_FLOAT)
#include <lslboost/multiprecision/cpp_dec_float.hpp>
#else
#include <lslboost/math/bindings/rr.hpp>
#endif

namespace lslboost{ namespace math{ namespace constants{ 

#ifdef USE_MPFR
typedef mpfr_class generator_type;
#elif defined(USE_MPREAL)
typedef mpfr::mpreal generator_type;
#elif defined(USE_CPP_FLOAT)
typedef lslboost::multiprecision::number<lslboost::multiprecision::cpp_dec_float<500> > generator_type;
#else
typedef ntl::RR generator_type;
#endif

inline void print_constant(const char* name, generator_type(*f)(const mpl::int_<0>&))
{
#ifdef USE_MPFR
   mpfr_class::set_dprec(((200 + 1) * 1000L) / 301L);
#elif defined(USE_MPREAL)
   mpfr::mpreal::set_default_prec(((200 + 1) * 1000L) / 301L);
#elif defined(USE_CPP_FLOAT)
   // Nothing to do, precision is already set.
#else
   ntl::RR::SetPrecision(((200 + 1) * 1000L) / 301L);
   ntl::RR::SetOutputPrecision(102);
#endif
   generator_type value = f(lslboost::mpl::int_<0>());
   std::stringstream os;
   os << std::setprecision(110) << std::scientific;
   os << value;
   std::string s = os.str();
   static const regex e("([+-]?\\d+(?:\\.\\d{0,36})?)(\\d*)(?:e([+-]?\\d+))?");
   smatch what;
   if(regex_match(s, what, e))
   {
      std::cout << 
         "BOOST_DEFINE_MATH_CONSTANT(" << name << ", " 
         << what[1] << "e" << (what[3].length() ? what[3].str() : std::string("0")) << ", " 
         << "\"" << what[1] << what[2] << "e" << (what[3].length() ? what[3].str() : std::string("0")) 
         << "\");" << std::endl;
   }
   else
   {
      std::cout << "Format of numeric constant was not recognised!!" << std::endl;
   }
}

#define BOOST_CONSTANTS_GENERATE(name) \
   lslboost::math::constants::print_constant(#name, \
   & lslboost::math::constants::detail::BOOST_JOIN(constant_, name)<lslboost::math::constants::generator_type>::get)

}}} // namespaces

#endif // BOOST_MATH_CONSTANTS_GENERATE_INCLUDED
