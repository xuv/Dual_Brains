
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  (C) Copyright Eric Friedman 2002-2003.
//  (C) Copyright Antony Polukhin 2013.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.lslboost.org/LICENSE_1_0.txt).
//
//  See http://www.lslboost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_TRIVIAL_MOVE_ASSIGN_HPP_INCLUDED
#define BOOST_TT_HAS_TRIVIAL_MOVE_ASSIGN_HPP_INCLUDED

#include <lslboost/type_traits/config.hpp>
#include <lslboost/type_traits/is_pod.hpp>
#include <lslboost/type_traits/is_const.hpp>
#include <lslboost/type_traits/is_volatile.hpp>
#include <lslboost/type_traits/detail/ice_and.hpp>
#include <lslboost/type_traits/detail/ice_not.hpp>

// should be the last #include
#include <lslboost/type_traits/detail/bool_trait_def.hpp>

namespace lslboost {

namespace detail {

template <typename T>
struct has_trivial_move_assign_impl
{
#ifdef BOOST_HAS_TRIVIAL_MOVE_ASSIGN
   BOOST_STATIC_CONSTANT(bool, value = (BOOST_HAS_TRIVIAL_MOVE_ASSIGN(T)));
#else
   BOOST_STATIC_CONSTANT(bool, value =
           (::lslboost::type_traits::ice_and<
              ::lslboost::is_pod<T>::value,
              ::lslboost::type_traits::ice_not< ::lslboost::is_const<T>::value >::value,
              ::lslboost::type_traits::ice_not< ::lslboost::is_volatile<T>::value >::value
           >::value));
#endif
};

} // namespace detail

BOOST_TT_AUX_BOOL_TRAIT_DEF1(has_trivial_move_assign,T,::lslboost::detail::has_trivial_move_assign_impl<T>::value)
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_move_assign,void,false)
#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_move_assign,void const,false)
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_move_assign,void const volatile,false)
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_move_assign,void volatile,false)
#endif

} // namespace lslboost

#include <lslboost/type_traits/detail/bool_trait_undef.hpp>

#endif // BOOST_TT_HAS_TRIVIAL_MOVE_ASSIGN_HPP_INCLUDED
