/* lslboost random/detail/seed.hpp header file
 *
 * Copyright Steven Watanabe 2009
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.lslboost.org/LICENSE_1_0.txt)
 *
 * See http://www.lslboost.org for most recent version including documentation.
 *
 * $Id: seed_impl.hpp 72951 2011-07-07 04:57:37Z steven_watanabe $
 */

#ifndef BOOST_RANDOM_DETAIL_SEED_IMPL_HPP
#define BOOST_RANDOM_DETAIL_SEED_IMPL_HPP

#include <stdexcept>
#include <lslboost/cstdint.hpp>
#include <lslboost/config/no_tr1/cmath.hpp>
#include <lslboost/integer/integer_mask.hpp>
#include <lslboost/integer/static_log2.hpp>
#include <lslboost/type_traits/is_signed.hpp>
#include <lslboost/type_traits/is_integral.hpp>
#include <lslboost/type_traits/make_unsigned.hpp>
#include <lslboost/mpl/bool.hpp>
#include <lslboost/mpl/if.hpp>
#include <lslboost/mpl/int.hpp>
#include <lslboost/random/detail/const_mod.hpp>
#include <lslboost/random/detail/integer_log2.hpp>
#include <lslboost/random/detail/signed_unsigned_tools.hpp>
#include <lslboost/random/detail/generator_bits.hpp>

#include <lslboost/random/detail/disable_warnings.hpp>

namespace lslboost {
namespace random {
namespace detail {

// finds the seed type of an engine, given its
// result_type.  If the result_type is integral
// the seed type is the same.  If the result_type
// is floating point, the seed type is uint32_t
template<class T>
struct seed_type
{
    typedef typename lslboost::mpl::if_<lslboost::is_integral<T>,
        T,
        lslboost::uint32_t
    >::type type;
};

template<int N>
struct const_pow_impl
{
    template<class T>
    static T call(T arg, int n, T result)
    {
        return const_pow_impl<N / 2>::call(arg * arg, n / 2,
            n%2 == 0? result : result * arg);
    }
};

template<>
struct const_pow_impl<0>
{
    template<class T>
    static T call(T, int, T result)
    {
        return result;
    }
};

// requires N is an upper bound on n
template<int N, class T>
inline T const_pow(T arg, int n) { return const_pow_impl<N>::call(arg, n, T(1)); }

template<class T>
inline T pow2(int n)
{
    typedef unsigned int_type;
    const int max_bits = std::numeric_limits<int_type>::digits;
    T multiplier = T(int_type(1) << (max_bits - 1)) * 2;
    return (int_type(1) << (n % max_bits)) *
        const_pow<std::numeric_limits<T>::digits / max_bits>(multiplier, n / max_bits);
}

template<class Engine, class Iter>
void generate_from_real(Engine& eng, Iter begin, Iter end)
{
    using std::fmod;
    typedef typename Engine::result_type RealType;
    const int Bits = detail::generator_bits<Engine>::value();
    int remaining_bits = 0;
    lslboost::uint_least32_t saved_bits = 0;
    RealType multiplier = pow2<RealType>( Bits);
    RealType mult32 = RealType(4294967296.0); // 2^32
    while(true) {
        RealType val = eng() * multiplier;
        int available_bits = Bits;
        // Make sure the compiler can optimize this out
        // if it isn't possible.
        if(Bits < 32 && available_bits < 32 - remaining_bits) {
            saved_bits |= lslboost::uint_least32_t(val) << remaining_bits;
            remaining_bits += Bits;
        } else {
            // If Bits < 32, then remaining_bits != 0, since
            // if remaining_bits == 0, available_bits < 32 - 0,
            // and we won't get here to begin with.
            if(Bits < 32 || remaining_bits != 0) {
                lslboost::uint_least32_t divisor =
                    (lslboost::uint_least32_t(1) << (32 - remaining_bits));
                lslboost::uint_least32_t extra_bits = lslboost::uint_least32_t(fmod(val, mult32)) & (divisor - 1);
                val = val / divisor;
                *begin++ = saved_bits | (extra_bits << remaining_bits);
                if(begin == end) return;
                available_bits -= 32 - remaining_bits;
                remaining_bits = 0;
            }
            // If Bits < 32 we should never enter this loop
            if(Bits >= 32) {
                for(; available_bits >= 32; available_bits -= 32) {
                    lslboost::uint_least32_t word = lslboost::uint_least32_t(fmod(val, mult32));
                    val /= mult32;
                    *begin++ = word;
                    if(begin == end) return;
                }
            }
            remaining_bits = available_bits;
            saved_bits = static_cast<lslboost::uint_least32_t>(val);
        }
    }
}

template<class Engine, class Iter>
void generate_from_int(Engine& eng, Iter begin, Iter end)
{
    typedef typename Engine::result_type IntType;
    typedef typename lslboost::make_unsigned<IntType>::type unsigned_type;
    int remaining_bits = 0;
    lslboost::uint_least32_t saved_bits = 0;
    unsigned_type range = lslboost::random::detail::subtract<IntType>()((eng.max)(), (eng.min)());

    int bits =
        (range == (std::numeric_limits<unsigned_type>::max)()) ?
            std::numeric_limits<unsigned_type>::digits :
            detail::integer_log2(range + 1);

    {
        int discarded_bits = detail::integer_log2(bits);
        unsigned_type excess = (range + 1) >> (bits - discarded_bits);
        if(excess != 0) {
            int extra_bits = detail::integer_log2((excess - 1) ^ excess);
            bits = bits - discarded_bits + extra_bits;
        }
    }

    unsigned_type mask = (static_cast<unsigned_type>(2) << (bits - 1)) - 1;
    unsigned_type limit = ((range + 1) & ~mask) - 1;

    while(true) {
        unsigned_type val;
        do {
            val = lslboost::random::detail::subtract<IntType>()(eng(), (eng.min)());
        } while(limit != range && val > limit);
        val &= mask;
        int available_bits = bits;
        if(available_bits == 32) {
            *begin++ = static_cast<lslboost::uint_least32_t>(val) & 0xFFFFFFFFu;
            if(begin == end) return;
        } else if(available_bits % 32 == 0) {
            for(int i = 0; i < available_bits / 32; ++i) {
                lslboost::uint_least32_t word = lslboost::uint_least32_t(val) & 0xFFFFFFFFu;
                int supress_warning = (bits >= 32);
                BOOST_ASSERT(supress_warning == 1);
                val >>= (32 * supress_warning);
                *begin++ = word;
                if(begin == end) return;
            }
        } else if(bits < 32 && available_bits < 32 - remaining_bits) {
            saved_bits |= lslboost::uint_least32_t(val) << remaining_bits;
            remaining_bits += bits;
        } else {
            if(bits < 32 || remaining_bits != 0) {
                lslboost::uint_least32_t extra_bits = lslboost::uint_least32_t(val) & ((lslboost::uint_least32_t(1) << (32 - remaining_bits)) - 1);
                val >>= 32 - remaining_bits;
                *begin++ = saved_bits | (extra_bits << remaining_bits);
                if(begin == end) return;
                available_bits -= 32 - remaining_bits;
                remaining_bits = 0;
            }
            if(bits >= 32) {
                for(; available_bits >= 32; available_bits -= 32) {
                    lslboost::uint_least32_t word = lslboost::uint_least32_t(val) & 0xFFFFFFFFu;
                    int supress_warning = (bits >= 32);
                    BOOST_ASSERT(supress_warning == 1);
                    val >>= (32 * supress_warning);
                    *begin++ = word;
                    if(begin == end) return;
                }
            }
            remaining_bits = available_bits;
            saved_bits = static_cast<lslboost::uint_least32_t>(val);
        }
    }
}

template<class Engine, class Iter>
void generate_impl(Engine& eng, Iter first, Iter last, lslboost::mpl::true_)
{
    return detail::generate_from_int(eng, first, last);
}

template<class Engine, class Iter>
void generate_impl(Engine& eng, Iter first, Iter last, lslboost::mpl::false_)
{
    return detail::generate_from_real(eng, first, last);
}

template<class Engine, class Iter>
void generate(Engine& eng, Iter first, Iter last)
{
    return detail::generate_impl(eng, first, last, lslboost::is_integral<typename Engine::result_type>());
}



template<class IntType, IntType m, class SeedSeq>
IntType seed_one_int(SeedSeq& seq)
{
    static const int log = ::lslboost::mpl::if_c<(m == 0),
        ::lslboost::mpl::int_<(::std::numeric_limits<IntType>::digits)>,
        ::lslboost::static_log2<m> >::type::value;
    static const int k =
        (log + ((~(static_cast<IntType>(2) << (log - 1)) & m)? 32 : 31)) / 32;
    ::lslboost::uint_least32_t array[log / 32 + 4];
    seq.generate(&array[0], &array[0] + k + 3);
    IntType s = 0;
    for(int j = 0; j < k; ++j) {
        IntType digit = const_mod<IntType, m>::apply(IntType(array[j+3]));
        IntType mult = IntType(1) << 32*j;
        s = const_mod<IntType, m>::mult_add(mult, digit, s);
    }
    return s;
}

template<class IntType, IntType m, class Iter>
IntType get_one_int(Iter& first, Iter last)
{
    static const int log = ::lslboost::mpl::if_c<(m == 0),
        ::lslboost::mpl::int_<(::std::numeric_limits<IntType>::digits)>,
        ::lslboost::static_log2<m> >::type::value;
    static const int k =
        (log + ((~(static_cast<IntType>(2) << (log - 1)) & m)? 32 : 31)) / 32;
    IntType s = 0;
    for(int j = 0; j < k; ++j) {
        if(first == last) {
            throw ::std::invalid_argument("Not enough elements in call to seed.");
        }
        IntType digit = const_mod<IntType, m>::apply(IntType(*first++));
        IntType mult = IntType(1) << 32*j;
        s = const_mod<IntType, m>::mult_add(mult, digit, s);
    }
    return s;
}

// TODO: work in-place whenever possible
template<int w, std::size_t n, class SeedSeq, class UIntType>
void seed_array_int_impl(SeedSeq& seq, UIntType (&x)[n])
{
    lslboost::uint_least32_t storage[((w+31)/32) * n];
    seq.generate(&storage[0], &storage[0] + ((w+31)/32) * n);
    for(std::size_t j = 0; j < n; j++) {
        UIntType val = 0;
        for(std::size_t k = 0; k < (w+31)/32; ++k) {
            val += static_cast<UIntType>(storage[(w+31)/32*j + k]) << 32*k;
        }
        x[j] = val & ::lslboost::low_bits_mask_t<w>::sig_bits;
    }
}

template<int w, std::size_t n, class SeedSeq, class IntType>
inline void seed_array_int_impl(SeedSeq& seq, IntType (&x)[n], lslboost::mpl::true_)
{
    typedef typename lslboost::make_unsigned<IntType>::type unsigned_array[n];
    seed_array_int_impl<w>(seq, reinterpret_cast<unsigned_array&>(x));
}

template<int w, std::size_t n, class SeedSeq, class IntType>
inline void seed_array_int_impl(SeedSeq& seq, IntType (&x)[n], lslboost::mpl::false_)
{
    seed_array_int_impl<w>(seq, x);
}

template<int w, std::size_t n, class SeedSeq, class IntType>
inline void seed_array_int(SeedSeq& seq, IntType (&x)[n])
{
    seed_array_int_impl<w>(seq, x, lslboost::is_signed<IntType>());
}

template<int w, std::size_t n, class Iter, class UIntType>
void fill_array_int_impl(Iter& first, Iter last, UIntType (&x)[n])
{
    for(std::size_t j = 0; j < n; j++) {
        UIntType val = 0;
        for(std::size_t k = 0; k < (w+31)/32; ++k) {
            if(first == last) {
                throw std::invalid_argument("Not enough elements in call to seed.");
            }
            val += static_cast<UIntType>(*first++) << 32*k;
        }
        x[j] = val & ::lslboost::low_bits_mask_t<w>::sig_bits;
    }
}

template<int w, std::size_t n, class Iter, class IntType>
inline void fill_array_int_impl(Iter& first, Iter last, IntType (&x)[n], lslboost::mpl::true_)
{
    typedef typename lslboost::make_unsigned<IntType>::type unsigned_array[n];
    fill_array_int_impl<w>(first, last, reinterpret_cast<unsigned_array&>(x));
}

template<int w, std::size_t n, class Iter, class IntType>
inline void fill_array_int_impl(Iter& first, Iter last, IntType (&x)[n], lslboost::mpl::false_)
{
    fill_array_int_impl<w>(first, last, x);
}

template<int w, std::size_t n, class Iter, class IntType>
inline void fill_array_int(Iter& first, Iter last, IntType (&x)[n])
{
    fill_array_int_impl<w>(first, last, x, lslboost::is_signed<IntType>());
}

template<int w, std::size_t n, class RealType>
void seed_array_real_impl(const lslboost::uint_least32_t* storage, RealType (&x)[n])
{
    lslboost::uint_least32_t mask = ~((~lslboost::uint_least32_t(0)) << (w%32));
    RealType two32 = 4294967296.0;
    const RealType divisor = RealType(1)/detail::pow2<RealType>(w);
    unsigned int j;
    for(j = 0; j < n; ++j) {
        RealType val = RealType(0);
        RealType mult = divisor;
        for(int k = 0; k < w/32; ++k) {
            val += *storage++ * mult;
            mult *= two32;
        }
        if(mask != 0) {
            val += (*storage++ & mask) * mult;
        }
        BOOST_ASSERT(val >= 0);
        BOOST_ASSERT(val < 1);
        x[j] = val;
    }
}

template<int w, std::size_t n, class SeedSeq, class RealType>
void seed_array_real(SeedSeq& seq, RealType (&x)[n])
{
    using std::pow;
    lslboost::uint_least32_t storage[((w+31)/32) * n];
    seq.generate(&storage[0], &storage[0] + ((w+31)/32) * n);
    seed_array_real_impl<w>(storage, x);
}

template<int w, std::size_t n, class Iter, class RealType>
void fill_array_real(Iter& first, Iter last, RealType (&x)[n])
{
    lslboost::uint_least32_t mask = ~((~lslboost::uint_least32_t(0)) << (w%32));
    RealType two32 = 4294967296.0;
    const RealType divisor = RealType(1)/detail::pow2<RealType>(w);
    unsigned int j;
    for(j = 0; j < n; ++j) {
        RealType val = RealType(0);
        RealType mult = divisor;
        for(int k = 0; k < w/32; ++k, ++first) {
            if(first == last) throw std::invalid_argument("Not enough elements in call to seed.");
            val += *first * mult;
            mult *= two32;
        }
        if(mask != 0) {
            if(first == last) throw std::invalid_argument("Not enough elements in call to seed.");
            val += (*first & mask) * mult;
            ++first;
        }
        BOOST_ASSERT(val >= 0);
        BOOST_ASSERT(val < 1);
        x[j] = val;
    }
}

}
}
}

#include <lslboost/random/detail/enable_warnings.hpp>

#endif
