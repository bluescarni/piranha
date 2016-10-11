/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_MP_INTEGER_HPP
#define PIRANHA_MP_INTEGER_HPP

#include <algorithm>
#include <array>
#include <boost/functional/hash.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <gmp.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.hpp"
#include "debug_access.hpp"
#include "detail/is_digit.hpp"
#include "detail/mp_rational_fwd.hpp"
#include "detail/mpfr.hpp"
#include "detail/real_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "detail/ulshift.hpp"
#include "exceptions.hpp"
#include "is_key.hpp"
#include "math.hpp"
#include "s11n.hpp"
#include "serialization.hpp"
#include "type_traits.hpp"

namespace piranha
{
namespace detail
{

// Small utility function to clear the upper n bits of an unsigned type.
// The static_casts are needed to work around integer promotions when
// operating on types smaller than unsigned int.
template <typename UInt>
inline UInt clear_top_bits(UInt input, unsigned n)
{
    static_assert(std::is_integral<UInt>::value && std::is_unsigned<UInt>::value, "Invalid type.");
    piranha_assert(n < unsigned(std::numeric_limits<UInt>::digits));
    return static_cast<UInt>(ulshift(input, n) >> n);
}

// Determine if the condition for using the optimised version
// of read_uint holds.
template <typename URet, unsigned IBits, unsigned RBits, typename UIn>
struct read_uint_opt {
    static const bool value = IBits == 0u && RBits == 0u && std::is_same<URet, UIn>::value;
};

// Considering an array of UIn (unsigned integrals) of a certain size as a continuous sequence of bits,
// read the index-th URet (unsigned integral) that can be extracted from the sequence of bits.
// The parameter IBits is the number of upper bits of UIn that should be discarded in the computation
// (i.e., not considered as part of the continuous sequence of bits). RBits has the same meaning,
// but for the output value: it's the number of upper bits that are not considered as part of the
// return type.
template <typename URet, unsigned IBits = 0u, unsigned RBits = 0u, typename UIn,
          typename std::enable_if<!read_uint_opt<URet, IBits, RBits, UIn>::value, int>::type = 0>
inline URet read_uint(const UIn *ptr, std::size_t size, std::size_t index)
{
    // We can work only with unsigned integer types.
    static_assert(std::is_integral<UIn>::value && std::is_unsigned<UIn>::value, "Invalid type.");
    static_assert(std::is_integral<URet>::value && std::is_unsigned<URet>::value, "Invalid type.");
    // Bit size of the return type.
    constexpr unsigned r_bits = static_cast<unsigned>(std::numeric_limits<URet>::digits);
    // Bit size of the input type.
    constexpr unsigned i_bits = static_cast<unsigned>(std::numeric_limits<UIn>::digits);
    // The ignored bits in the input type cannot be larger than or equal to its bit size.
    static_assert(IBits < i_bits, "Invalid ignored input bits size");
    // Same for for RBits.
    static_assert(RBits < r_bits, "Invalid ignored output bits size");
    // Bits effectively considered in the input type.
    constexpr unsigned ei_bits = i_bits - IBits;
    // Bits effectively considered in the output type.
    constexpr unsigned er_bits = r_bits - RBits;
    // Index in input array from where we will start reading, and bit index within
    // that element from which the actual reading will start.
    // NOTE: we need to protect against multiplication overflows in the upper level routine.
    std::size_t s_index = static_cast<std::size_t>((er_bits * index) / ei_bits),
                r_index = static_cast<std::size_t>((er_bits * index) % ei_bits);
    // Check that we are not going to read past the end.
    piranha_assert(s_index < size);
    // Check for null.
    piranha_assert(ptr != nullptr);
    // Init the return value as 0.
    URet retval = 0u;
    // Bits read so far.
    unsigned read_bits = 0u;
    while (s_index < size && read_bits < er_bits) {
        const unsigned unread_bits = er_bits - read_bits,
                       // This can be different from ei_bits only on the first iteration.
            available_bits = static_cast<unsigned>(ei_bits - r_index),
                       bits_to_read = (available_bits > unread_bits) ? unread_bits : available_bits;
        // On the first iteration, we might need to drop the first r_index bits.
        UIn tmp = static_cast<UIn>(ptr[s_index] >> r_index);
        // Zero out the bits we don't want to read.
        tmp = clear_top_bits(tmp, i_bits - bits_to_read);
        // Accumulate into return value.
        retval = static_cast<URet>(retval + (static_cast<URet>(tmp) << read_bits));
        // Update loop variables.
        read_bits += bits_to_read;
        ++s_index;
        r_index = 0u;
    }
    piranha_assert(s_index == size || read_bits == er_bits);
    return retval;
}

// This is an optimised version of the above, kicking in when:
// - we do not ignore any bit in input or output,
// - in and out types are the same.
// In this case, we can read directly the value from the pointer.
template <typename URet, unsigned IBits = 0u, unsigned RBits = 0u, typename UIn,
          typename std::enable_if<read_uint_opt<URet, IBits, RBits, UIn>::value, int>::type = 0>
inline URet read_uint(const UIn *ptr, std::size_t size, std::size_t index)
{
    (void)size;
    // We can work only with unsigned integer types.
    static_assert(std::is_integral<UIn>::value && std::is_unsigned<UIn>::value, "Invalid type.");
    static_assert(std::is_integral<URet>::value && std::is_unsigned<URet>::value, "Invalid type.");
    // Check that we are not going to read past the end.
    piranha_assert(index < size);
    // Check for null.
    piranha_assert(ptr != nullptr);
    return ptr[index];
}

// mpz_t is an array of some struct.
using mpz_struct_t = std::remove_extent<::mpz_t>::type;
// Integral types used for allocation size and number of limbs.
using mpz_alloc_t = decltype(std::declval<mpz_struct_t>()._mp_alloc);
using mpz_size_t = decltype(std::declval<mpz_struct_t>()._mp_size);

// Some misc tests to check that the mpz struct conforms to our expectations.
// This is crucial for the implementation of the union integer type.
struct expected_mpz_struct_t {
    mpz_alloc_t _mp_alloc;
    mpz_size_t _mp_size;
    ::mp_limb_t *_mp_d;
};

static_assert(sizeof(expected_mpz_struct_t) == sizeof(mpz_struct_t) && std::is_standard_layout<mpz_struct_t>::value
                  && std::is_standard_layout<expected_mpz_struct_t>::value && offsetof(mpz_struct_t, _mp_alloc) == 0u
                  && offsetof(mpz_struct_t, _mp_size) == offsetof(expected_mpz_struct_t, _mp_size)
                  && offsetof(mpz_struct_t, _mp_d) == offsetof(expected_mpz_struct_t, _mp_d)
                  && std::is_same<mpz_alloc_t, decltype(std::declval<mpz_struct_t>()._mp_alloc)>::value
                  && std::is_same<mpz_size_t, decltype(std::declval<mpz_struct_t>()._mp_size)>::value
                  && std::is_same<::mp_limb_t *, decltype(std::declval<mpz_struct_t>()._mp_d)>::value &&
                  // mp_bitcnt_t is used in shift operators, so we double-check it is unsigned. If it is not
                  // we might end up shifting by negative values, which is UB.
                  std::is_unsigned<::mp_bitcnt_t>::value &&
                  // Sanity check on GMP_LIMB_BITS. We use both GMP_LIMB_BITS and numeric_limits interchangeably,
                  // e.g., when using read_uint.
                  std::numeric_limits<::mp_limb_t>::digits == unsigned(GMP_LIMB_BITS),
              "Invalid mpz_t struct layout and/or GMP types.");

// Metaprogramming to select the limb/dlimb types.
template <int NBits>
struct si_limb_types {
    static_assert(NBits == 0, "Invalid limb size.");
};

#if defined(PIRANHA_UINT128_T)
// NOTE: we are lacking native 128 bit types on MSVC, it should be possible to implement them
// though using primitives like this:
// http://msdn.microsoft.com/en-us/library/windows/desktop/hh802931(v=vs.85).aspx
template <>
struct si_limb_types<64> {
    using limb_t = std::uint_least64_t;
    using dlimb_t = PIRANHA_UINT128_T;
    static_assert(static_cast<dlimb_t>(std::numeric_limits<limb_t>::max())
                      <= -dlimb_t(1) / std::numeric_limits<limb_t>::max(),
                  "128-bit integer is too narrow.");
    static const limb_t limb_bits = 64;
};
#endif

template <>
struct si_limb_types<32> {
    using limb_t = std::uint_least32_t;
    using dlimb_t = std::uint_least64_t;
    static const limb_t limb_bits = 32;
};

template <>
struct si_limb_types<16> {
    using limb_t = std::uint_least16_t;
    using dlimb_t = std::uint_least32_t;
    static const limb_t limb_bits = 16;
};

template <>
struct si_limb_types<8> {
    using limb_t = std::uint_least8_t;
    using dlimb_t = std::uint_least16_t;
    static const limb_t limb_bits = 8;
};

template <>
struct si_limb_types<0> : public si_limb_types<
#if defined(PIRANHA_UINT128_T)
                              64
#else
                              32
#endif
                              > {
};

// Simple RAII holder for GMP integers.
struct mpz_raii {
    mpz_raii()
    {
        ::mpz_init(&m_mpz);
        piranha_assert(m_mpz._mp_alloc >= 0);
    }
    mpz_raii(const mpz_raii &) = delete;
    mpz_raii(mpz_raii &&) = delete;
    mpz_raii &operator=(const mpz_raii &) = delete;
    mpz_raii &operator=(mpz_raii &&) = delete;
    ~mpz_raii()
    {
        // NOTE: even in recent GMP versions, with lazy allocation,
        // it seems like the pointer always points to something:
        // https://gmplib.org/repo/gmp/file/835f8974ff6e/mpz/init.c
        piranha_assert(m_mpz._mp_d != nullptr);
        ::mpz_clear(&m_mpz);
    }
    mpz_struct_t m_mpz;
};

inline std::ostream &stream_mpz(std::ostream &os, const mpz_struct_t &mpz)
{
    const std::size_t size_base10 = ::mpz_sizeinbase(&mpz, 10);
    if (unlikely(size_base10 > std::numeric_limits<std::size_t>::max() - static_cast<std::size_t>(2))) {
        piranha_throw(std::invalid_argument, "number of digits is too large");
    }
    const auto total_size = size_base10 + 2u;
    PIRANHA_MAYBE_TLS std::vector<char> tmp;
    tmp.resize(static_cast<std::vector<char>::size_type>(total_size));
    if (unlikely(tmp.size() != total_size)) {
        piranha_throw(std::invalid_argument, "number of digits is too large");
    }
    os << ::mpz_get_str(&tmp[0u], 10, &mpz);
    return os;
}

template <int NBits>
struct static_integer {
    using dlimb_t = typename si_limb_types<NBits>::dlimb_t;
    using limb_t = typename si_limb_types<NBits>::limb_t;
    // Limb bits used for the representation of the number.
    static const limb_t limb_bits = si_limb_types<NBits>::limb_bits;
    // Total number of bits in the limb type, >= limb_bits.
    static const unsigned total_bits = static_cast<unsigned>(std::numeric_limits<limb_t>::digits);
    static_assert(total_bits >= limb_bits, "Invalid limb_t type.");
    using limbs_type = std::array<limb_t, std::size_t(2)>;
    // Check: we need to be able to address all bits in the 2 limbs using limb_t.
    static_assert(limb_bits < std::numeric_limits<limb_t>::max() / 2u, "Overflow error.");
    // NOTE: init everything otherwise zero is gonna be represented by undefined values in lo/hi.
    static_integer() : _mp_alloc(-1), _mp_size(0), m_limbs()
    {
    }
    template <typename T,
              typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, int>::type = 0>
    bool attempt_direct_construction(T n)
    {
        try {
            if (n >= T(0)) {
                // We can attempt conversion of a non-negative signed directly into limb_t.
                m_limbs[0u] = boost::numeric_cast<limb_t>(n);
            } else if (n >= -safe_abs_sint<T>::value) {
                // Conversion of a negative signed whose value can be safely negated.
                m_limbs[0u] = boost::numeric_cast<limb_t>(-n);
            } else {
                // We cannot convert safely n to a limb_t in this case.
                return false;
            }
            if (n > T(0)) {
                // Strictly positive value, size is 1.
                _mp_size = mpz_size_t(1);
            }
            if (n < T(0)) {
                // Strictly negative value, size is -1.
                _mp_size = mpz_size_t(-1);
            }
            // NOTE: if n is zero, size has been inited to zero already.
            return true;
        } catch (...) {
            return false;
        }
    }
    template <typename T,
              typename std::enable_if<std::is_unsigned<T>::value && std::is_integral<T>::value, int>::type = 0>
    bool attempt_direct_construction(T n)
    {
        try {
            // For unsigned, directly attempt a cast to the limb type.
            m_limbs[0u] = boost::numeric_cast<limb_t>(n);
            // With an unsigned input, the size can be either 0 or 1.
            if (n) {
                _mp_size = mpz_size_t(1);
            }
            return true;
        } catch (...) {
            return false;
        }
    }
    template <typename Integer>
    void construct_from_integral(Integer n)
    {
        // Don't do anything if n is zero or is the direct construction
        // into the first limb succeeds.
        if (!n || likely(attempt_direct_construction(n))) {
            return;
        }
        const auto orig_n = n;
        limb_t bit_idx = 0;
        while (n != Integer(0)) {
            if (bit_idx == limb_bits * 2u) {
                // Clear out before throwing, as this is used in mp_integer as well.
                _mp_size = 0;
                m_limbs[0u] = 0u;
                m_limbs[1u] = 0u;
                piranha_throw(std::overflow_error, "insufficient bit width");
            }
            // NOTE: in C++11 division will round to zero always (for negative numbers as well).
            // The bit shift operator >> has implementation defined behaviour if n is signed and negative,
            // so we do not use it.
            const Integer quot = static_cast<Integer>(n / Integer(2)), rem = static_cast<Integer>(n % Integer(2));
            if (rem) {
                set_bit(bit_idx);
            }
            n = quot;
            ++bit_idx;
        }
        fix_sign_ctor(orig_n);
    }
    template <typename Integer, typename = typename std::enable_if<std::is_integral<Integer>::value>::type>
    explicit static_integer(Integer n) : _mp_alloc(-1), _mp_size(0), m_limbs()
    {
        construct_from_integral(n);
    }
    template <typename T>
    void fix_sign_ctor(T, typename std::enable_if<std::is_unsigned<T>::value>::type * = nullptr)
    {
    }
    template <typename T>
    void fix_sign_ctor(T n, typename std::enable_if<std::is_signed<T>::value>::type * = nullptr)
    {
        if (n < T(0)) {
            negate();
        }
    }
    static_integer(const static_integer &) = default;
    static_integer(static_integer &&) = default;
    ~static_integer()
    {
        piranha_assert(consistency_checks());
    }
    static_integer &operator=(const static_integer &) = default;
    static_integer &operator=(static_integer &&) = default;
    void negate()
    {
        // NOTE: this is 2 at most, no danger in taking the negative.
        _mp_size = -_mp_size;
    }
    void set_bit(const limb_t &idx)
    {
        using size_type = typename limbs_type::size_type;
        piranha_assert(idx < limb_bits * 2u);
        // Crossing fingers for compiler optimising this out.
        const auto quot = static_cast<limb_t>(idx / limb_bits), rem = static_cast<limb_t>(idx % limb_bits);
        m_limbs[static_cast<size_type>(quot)]
            = static_cast<limb_t>(m_limbs[static_cast<size_type>(quot)] | static_cast<limb_t>(limb_t(1) << rem));
        // Update the size if needed. The new size must be at least quot + 1, as we set a bit
        // in the limb with index quot.
        const auto new_size = static_cast<mpz_size_t>(quot + 1u);
        if (_mp_size < 0) {
            if (-new_size < _mp_size) {
                _mp_size = -new_size;
            }
        } else {
            if (new_size > _mp_size) {
                _mp_size = new_size;
            }
        }
    }
    mpz_size_t calculate_n_limbs() const
    {
        if (m_limbs[1u] != 0u) {
            return 2;
        }
        if (m_limbs[0u] != 0u) {
            return 1;
        }
        return 0;
    }
    bool consistency_checks() const
    {
        return _mp_alloc == mpz_alloc_t(-1) && _mp_size <= 2 && _mp_size >= -2 &&
               // Excess bits must be zero for consistency.
               !(static_cast<dlimb_t>(m_limbs[0u]) >> limb_bits) && !(static_cast<dlimb_t>(m_limbs[1u]) >> limb_bits)
               && (calculate_n_limbs() == _mp_size || -calculate_n_limbs() == _mp_size);
    }
    mpz_size_t abs_size() const
    {
        return static_cast<mpz_size_t>((_mp_size >= 0) ? _mp_size : -_mp_size);
    }
    // Read-only mpz view class. After creation, this class can be used
    // as const mpz_t argument in GMP functions, thanks to the implicit conversion
    // operator.
    template <typename T = static_integer, typename = void>
    class static_mpz_view
    {
    public:
        // Safe, checked above.
        static const auto max_tot_nbits = 2u * T::limb_bits;
        // Check the conversion below.
        static_assert(max_tot_nbits / unsigned(GMP_NUMB_BITS) + 1u <= std::numeric_limits<std::size_t>::max(),
                      "Overflow error.");
        // Maximum number of GMP limbs to use. This will define the size of the static
        // storage below, m_limbs. The number of actual limbs needed to represent
        // the static integer will never be greater than this number.
        static const std::size_t max_n_gmp_limbs = static_cast<std::size_t>(
            max_tot_nbits % unsigned(GMP_NUMB_BITS) == 0u ? max_tot_nbits / unsigned(GMP_NUMB_BITS)
                                                          : max_tot_nbits / unsigned(GMP_NUMB_BITS) + 1u);
        static_assert(max_n_gmp_limbs >= 1u, "Invalid number of GMP limbs.");
        // NOTE: this is needed when we have the variant view in the integer class: if the active view
        // is the dynamic one, we need to def construct a static view that we will never use.
        static_mpz_view() : m_mpz(), m_limbs()
        {
        }
        explicit static_mpz_view(const static_integer &n)
        {
            std::size_t asize;
            bool sign;
            if (n._mp_size < 0) {
                sign = false;
                asize = static_cast<std::size_t>(-n._mp_size);
            } else {
                sign = true;
                asize = static_cast<std::size_t>(n._mp_size);
            }
            piranha_assert(asize <= 2u);
            const auto tot_nbits = asize * T::limb_bits;
            const std::size_t n_gmp_limbs = static_cast<std::size_t>(tot_nbits % unsigned(GMP_NUMB_BITS) == 0u
                                                                         ? tot_nbits / unsigned(GMP_NUMB_BITS)
                                                                         : tot_nbits / unsigned(GMP_NUMB_BITS) + 1u);
            // Overflow check when using read_uint.
            static_assert(unsigned(GMP_LIMB_BITS) <= std::numeric_limits<std::size_t>::max() / max_n_gmp_limbs,
                          "Overflow error.");
            for (std::size_t i = 0u; i < n_gmp_limbs; ++i) {
                m_limbs[i]
                    = read_uint<::mp_limb_t, T::total_bits - T::limb_bits, unsigned(GMP_LIMB_BITS - GMP_NUMB_BITS)>(
                        n.m_limbs.data(), asize, i);
            }
            // Fill in the missing limbs, otherwise we have indeterminate values.
            for (std::size_t i = n_gmp_limbs; i < max_n_gmp_limbs; ++i) {
                m_limbs[i] = 0u;
            }
            // Final assignment.
            static_assert(max_n_gmp_limbs <= static_cast<std::make_unsigned<mpz_alloc_t>::type>(
                                                 std::numeric_limits<mpz_alloc_t>::max()),
                          "Overflow error.");
            // The allocated size of the fictitious mpz struct will be the size of the static storage.
            m_mpz._mp_alloc = static_cast<mpz_alloc_t>(max_n_gmp_limbs);
            static_assert(max_n_gmp_limbs <= static_cast<std::make_unsigned<mpz_size_t>::type>(
                                                 detail::safe_abs_sint<mpz_size_t>::value),
                          "Overflow error.");
            m_mpz._mp_size = sign ? static_cast<mpz_size_t>(n_gmp_limbs) : -static_cast<mpz_size_t>(n_gmp_limbs);
            m_mpz._mp_d = m_limbs.data();
        }
        // Leave only the move ctor so that this can be returned by a function.
        static_mpz_view(const static_mpz_view &) = delete;
        static_mpz_view(static_mpz_view &&) = default;
        static_mpz_view &operator=(const static_mpz_view &) = delete;
        static_mpz_view &operator=(static_mpz_view &&) = delete;
        operator const mpz_struct_t *() const
        {
            return &m_mpz;
        }

    private:
        mpz_struct_t m_mpz;
        std::array<::mp_limb_t, max_n_gmp_limbs> m_limbs;
    };
    // NOTE: in order to activate this optimisation, we need to be certain that:
    // - the limbs type coincide, as we are doing a const_cast between them,
    // - the number of bits effectively used is identical.
    template <typename T>
    class static_mpz_view<T, typename std::enable_if<std::is_same<::mp_limb_t, typename T::limb_t>::value
                                                     && T::limb_bits == unsigned(GMP_NUMB_BITS)>::type>
    {
    public:
        static_mpz_view() : m_mpz()
        {
        }
        // NOTE: we use the const_cast to cast away the constness from the pointer to the limbs
        // in n. This is valid as we are never going to use this pointer for writing.
        explicit static_mpz_view(const static_integer &n)
            : m_mpz{static_cast<mpz_alloc_t>(2), n._mp_size, const_cast<::mp_limb_t *>(n.m_limbs.data())}
        {
        }
        static_mpz_view(const static_mpz_view &) = delete;
        static_mpz_view(static_mpz_view &&) = default;
        static_mpz_view &operator=(const static_mpz_view &) = delete;
        static_mpz_view &operator=(static_mpz_view &&) = delete;
        operator const mpz_struct_t *() const
        {
            return &m_mpz;
        }

    private:
        mpz_struct_t m_mpz;
    };
    static_mpz_view<> get_mpz_view() const
    {
        return static_mpz_view<>{*this};
    }
    friend std::ostream &operator<<(std::ostream &os, const static_integer &si)
    {
        auto v = si.get_mpz_view();
        return stream_mpz(os, *static_cast<const mpz_struct_t *>(v));
    }
    bool operator==(const static_integer &other) const
    {
        return _mp_size == other._mp_size && m_limbs == other.m_limbs;
    }
    bool operator!=(const static_integer &other) const
    {
        return !operator==(other);
    }
    bool is_zero() const
    {
        return _mp_size == 0;
    }
    bool is_unitary() const
    {
        return _mp_size == 1 && m_limbs[0u] == 1u;
    }
    // Compare absolute values of two integers whose sizes are the same in absolute value.
    static int compare(const static_integer &a, const static_integer &b, const mpz_size_t &size)
    {
        using size_type = typename limbs_type::size_type;
        piranha_assert(size >= 0 && size <= 2);
        piranha_assert(a._mp_size == size || -a._mp_size == size);
        piranha_assert(a._mp_size == b._mp_size || a._mp_size == -b._mp_size);
        auto limb_idx = static_cast<size_type>(size);
        while (limb_idx != 0u) {
            --limb_idx;
            if (a.m_limbs[limb_idx] > b.m_limbs[limb_idx]) {
                return 1;
            } else if (a.m_limbs[limb_idx] < b.m_limbs[limb_idx]) {
                return -1;
            }
        }
        return 0;
    }
    bool operator<(const static_integer &other) const
    {
        const auto size0 = _mp_size;
        const auto size1 = other._mp_size;
        if (size0 < size1) {
            return true;
        } else if (size1 < size0) {
            return false;
        } else {
            const mpz_size_t abs_size = static_cast<mpz_size_t>(size0 >= 0 ? size0 : -size0);
            const int cmp = compare(*this, other, abs_size);
            return (size0 >= 0) ? cmp < 0 : cmp > 0;
        }
    }
    bool operator>(const static_integer &other) const
    {
        const auto size0 = _mp_size;
        const auto size1 = other._mp_size;
        if (size0 < size1) {
            return false;
        } else if (size1 < size0) {
            return true;
        } else {
            const mpz_size_t abs_size = static_cast<mpz_size_t>(size0 >= 0 ? size0 : -size0);
            const int cmp = compare(*this, other, abs_size);
            return (size0 >= 0) ? cmp > 0 : cmp < 0;
        }
    }
    bool operator>=(const static_integer &other) const
    {
        return !operator<(other);
    }
    bool operator<=(const static_integer &other) const
    {
        return !operator>(other);
    }
    // NOTE: the idea here is that it could happen that limb_bits is smaller than the actual total bits
    // used for the representation of limb_t. In such a case, in arithmetic operations whenever we cast from
    // dlimb_t to limb_t or exploit wrap-around arithmetics, we might have extra bits past limb_bits set
    // that need to be set to zero for consistency.
    template <typename T = static_integer>
    void clear_extra_bits(typename std::enable_if<T::limb_bits != T::total_bits>::type * = nullptr)
    {
        const auto delta_bits = total_bits - limb_bits;
        m_limbs[0u] = clear_top_bits(m_limbs[0u], delta_bits);
        m_limbs[1u] = clear_top_bits(m_limbs[1u], delta_bits);
    }
    template <typename T = static_integer>
    void clear_extra_bits(typename std::enable_if<T::limb_bits == T::total_bits>::type * = nullptr)
    {
    }
    static int raw_add(static_integer &res, const static_integer &x, const static_integer &y)
    {
        piranha_assert(x.abs_size() <= 2 && y.abs_size() <= 2);
        const dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(x.m_limbs[0u]) + y.m_limbs[0u]);
        const dlimb_t hi
            = static_cast<dlimb_t>((static_cast<dlimb_t>(x.m_limbs[1u]) + y.m_limbs[1u]) + (lo >> limb_bits));
        // NOTE: exit before modifying anything here, so that res is not modified.
        if (unlikely(static_cast<limb_t>(hi >> limb_bits) != 0u)) {
            return 1;
        }
        res.m_limbs[0u] = static_cast<limb_t>(lo);
        res.m_limbs[1u] = static_cast<limb_t>(hi);
        res._mp_size = res.calculate_n_limbs();
        res.clear_extra_bits();
        return 0;
    }
    static void raw_sub(static_integer &res, const static_integer &x, const static_integer &y)
    {
        piranha_assert(x.abs_size() <= 2 && y.abs_size() <= 2);
        piranha_assert(x.abs_size() >= y.abs_size());
        piranha_assert(x.m_limbs[1u] >= y.m_limbs[1u]);
        const bool has_borrow = x.m_limbs[0u] < y.m_limbs[0u];
        piranha_assert(x.m_limbs[1u] > y.m_limbs[1u] || !has_borrow);
        res.m_limbs[0u] = static_cast<limb_t>(x.m_limbs[0u] - y.m_limbs[0u]);
        res.m_limbs[1u] = static_cast<limb_t>((x.m_limbs[1u] - y.m_limbs[1u]) - limb_t(has_borrow));
        res._mp_size = res.calculate_n_limbs();
        res.clear_extra_bits();
    }
    template <bool AddOrSub>
    static int add_or_sub(static_integer &res, const static_integer &x, const static_integer &y)
    {
        mpz_size_t asizex = x._mp_size, asizey = static_cast<mpz_size_t>(AddOrSub ? y._mp_size : -y._mp_size);
        bool signx = true, signy = true;
        if (asizex < 0) {
            asizex = -asizex;
            signx = false;
        }
        if (asizey < 0) {
            asizey = -asizey;
            signy = false;
        }
        piranha_assert(asizex <= 2 && asizey <= 2);
        if (signx == signy) {
            if (unlikely(raw_add(res, x, y))) {
                return 1;
            }
            if (!signx) {
                res.negate();
            }
        } else {
            if (asizex > asizey || (asizex == asizey && compare(x, y, asizex) >= 0)) {
                raw_sub(res, x, y);
                if (!signx) {
                    res.negate();
                }
            } else {
                raw_sub(res, y, x);
                if (!signy) {
                    res.negate();
                }
            }
        }
        return 0;
    }
    static int add(static_integer &res, const static_integer &x, const static_integer &y)
    {
        return add_or_sub<true>(res, x, y);
    }
    static int sub(static_integer &res, const static_integer &x, const static_integer &y)
    {
        return add_or_sub<false>(res, x, y);
    }
    static void raw_mul(static_integer &res, const static_integer &x, const static_integer &y, const mpz_size_t &asizex,
                        const mpz_size_t &asizey)
    {
        piranha_assert(asizex > 0 && asizey > 0);
        const dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(x.m_limbs[0u]) * y.m_limbs[0u]);
        res.m_limbs[0u] = static_cast<limb_t>(lo);
        const limb_t cy_limb = static_cast<limb_t>(lo >> limb_bits);
        res.m_limbs[1u] = cy_limb;
        res._mp_size = static_cast<mpz_size_t>((asizex + asizey) - mpz_size_t(cy_limb == 0u));
        res.clear_extra_bits();
        piranha_assert(res._mp_size > 0);
    }
    static int mul(static_integer &res, const static_integer &x, const static_integer &y)
    {
        mpz_size_t asizex = x._mp_size, asizey = y._mp_size;
        if (unlikely(asizex == 0 || asizey == 0)) {
            res._mp_size = 0;
            res.m_limbs[0u] = 0u;
            res.m_limbs[1u] = 0u;
            return 0;
        }
        bool signx = true, signy = true;
        if (asizex < 0) {
            asizex = -asizex;
            signx = false;
        }
        if (asizey < 0) {
            asizey = -asizey;
            signy = false;
        }
        if (unlikely(asizex > 1 || asizey > 1)) {
            return 1;
        }
        raw_mul(res, x, y, asizex, asizey);
        if (signx != signy) {
            res.negate();
        }
        return 0;
    }
    static_integer &operator+=(const static_integer &other)
    {
        add(*this, *this, other);
        return *this;
    }
    static_integer &operator-=(const static_integer &other)
    {
        sub(*this, *this, other);
        return *this;
    }
    static_integer &operator*=(const static_integer &other)
    {
        mul(*this, *this, other);
        return *this;
    }
    static_integer operator+() const
    {
        return *this;
    }
    friend static_integer operator+(const static_integer &x, const static_integer &y)
    {
        static_integer retval(x);
        retval += y;
        return retval;
    }
    static_integer operator-() const
    {
        static_integer retval(*this);
        retval.negate();
        return retval;
    }
    friend static_integer operator-(const static_integer &x, const static_integer &y)
    {
        static_integer retval(x);
        retval -= y;
        return retval;
    }
    friend static_integer operator*(const static_integer &x, const static_integer &y)
    {
        static_integer retval(x);
        retval *= y;
        return retval;
    }
    int multiply_accumulate(const static_integer &b, const static_integer &c)
    {
        mpz_size_t asizea = _mp_size, asizeb = b._mp_size, asizec = c._mp_size;
        bool signa = true, signb = true, signc = true;
        if (asizea < 0) {
            asizea = -asizea;
            signa = false;
        }
        if (asizeb < 0) {
            asizeb = -asizeb;
            signb = false;
        }
        if (asizec < 0) {
            asizec = -asizec;
            signc = false;
        }
        piranha_assert(asizea <= 2);
        if (unlikely(asizeb > 1 || asizec > 1)) {
            return 1;
        }
        if (unlikely(asizeb == 0 || asizec == 0)) {
            return 0;
        }
        static_integer tmp;
        raw_mul(tmp, b, c, asizeb, asizec);
        const mpz_size_t asizetmp = tmp._mp_size;
        const bool signtmp = (signb == signc);
        piranha_assert(asizetmp <= 2 && asizetmp > 0);
        if (signa == signtmp) {
            if (unlikely(raw_add(*this, *this, tmp))) {
                return 1;
            }
            if (!signa) {
                negate();
            }
        } else {
            if (asizea > asizetmp || (asizea == asizetmp && compare(*this, tmp, asizea) >= 0)) {
                raw_sub(*this, *this, tmp);
                if (!signa) {
                    negate();
                }
            } else {
                raw_sub(*this, tmp, *this);
                if (!signtmp) {
                    negate();
                }
            }
        }
        return 0;
    }
    // lshift by n bits.
    int lshift(::mp_bitcnt_t n)
    {
        // Shift by zero or on a zero value has no effect
        // and it is always successful.
        if (n == 0u || _mp_size == 0) {
            return 0;
        }
        // Too much shift, this can never work on a nonzero value.
        if (n >= 2u * limb_bits) {
            return 1;
        }
        limb_t hi;
        limb_t lo;
        if (n >= limb_bits) {
            if (m_limbs[1u] != 0u) {
                // Too much shift.
                return 1;
            }
            // Move lo to hi and set lo to zero.
            hi = m_limbs[0u];
            lo = 0u;
            if (n == limb_bits) {
                // If n is exactly limb_bits,
                // we can assign the new hi/lo and stop here.
                m_limbs[0u] = lo;
                m_limbs[1u] = hi;
                // The size has to be 2.
                _mp_size = (_mp_size > 0) ? static_cast<mpz_size_t>(2) : static_cast<mpz_size_t>(-2);
                return 0;
            }
            // Moving lo to hi and setting lo to zero
            // means we already shifted by limb_bits.
            n = static_cast<::mp_bitcnt_t>(n - limb_bits);
        } else {
            // hi and lo are the original ones.
            lo = m_limbs[0u];
            hi = m_limbs[1u];
        }
        // Check that hi will not be shifted too much. Note that
        // here and below n can never be zero, so we never shift too much.
        piranha_assert(n > 0u);
        if (hi >= (limb_t(1) << (limb_bits - n))) {
            return 1;
        }
        // Shift hi and lo. hi gets the carry over from lo.
        hi = static_cast<limb_t>(ulshift(hi, n) + (lo >> (limb_bits - n)));
        // NOTE: ulshift makes sure the operation is well defined also for
        // short integral types. This will be a no-op if lo has been set to zero
        // above.
        lo = ulshift(lo, n);
        // Assign back hi and lo.
        m_limbs[0u] = lo;
        m_limbs[1u] = hi;
        // Now update the size.
        bool sign = true;
        if (_mp_size < 0) {
            sign = false;
        }
        // We know the size is at least 1 as the case of size 0 was already
        // handled in the beginning.
        piranha_assert(abs_size() >= 1);
        mpz_size_t asize = 1;
        asize = static_cast<mpz_size_t>(asize + static_cast<mpz_size_t>(m_limbs[1u] != 0u));
        _mp_size = static_cast<mpz_size_t>(sign ? asize : -asize);
        // Extra bits could in principle be remaining in lo after the left shift.
        clear_extra_bits();
        return 0;
    }
    // rshift by n bits.
    void rshift(::mp_bitcnt_t n)
    {
        // Shift by zero or on a zero value has no effect.
        if (n == 0u || _mp_size == 0) {
            return;
        }
        // Shift by 2 * limb_bits or greater will produce zero.
        if (n >= 2u * limb_bits) {
            _mp_size = 0;
            m_limbs[0u] = 0u;
            m_limbs[1u] = 0u;
            return;
        }
        if (n >= limb_bits) {
            // NOTE: this is ok if n == limb_bits, no special casing needed.
            m_limbs[0u] = static_cast<limb_t>(m_limbs[1u] >> (n - limb_bits));
            m_limbs[1u] = 0u;
            // The size could be zero or +-1, depending
            // on the new content of m_limbs[0] and the previous
            // sign of _mp_size.
            _mp_size = (m_limbs[0u] == 0u) ? mpz_size_t(0) : static_cast<mpz_size_t>((_mp_size > 0) ? 1 : -1);
            return;
        }
        // Here we know that 0 < n < limb_bits.
        piranha_assert(n > 0u && n < limb_bits);
        // This represents the bits in hi that will be shifted down into lo.
        // We move them up so we can add tmp to the new lo to account for them.
        // NOTE: here we could have excess bits if limb_bits != total_bits, we will
        // clean up below with clear_extra_bits().
        const limb_t tmp(ulshift(m_limbs[1u], limb_bits - n));
        m_limbs[0u] = static_cast<limb_t>((m_limbs[0u] >> n) + tmp);
        m_limbs[1u] = static_cast<limb_t>(m_limbs[1u] >> n);
        clear_extra_bits();
        const auto new_asize = calculate_n_limbs();
        _mp_size = static_cast<mpz_size_t>((_mp_size > 0) ? new_asize : -new_asize);
    }
    // Division.
    static void div(static_integer &q, static_integer &r, const static_integer &a, const static_integer &b)
    {
        piranha_assert(!b.is_zero());
        // NOTE: here in principle q/r could overlap with a or b (e.g., in in-place division).
        // We need to first read everything we need from a and b, and only then write into q/r.
        // Store the signs.
        const bool signa = a._mp_size >= 0, signb = b._mp_size >= 0;
        // Compute the result in dlimb_t.
        const dlimb_t ad = static_cast<dlimb_t>(a.m_limbs[0u] + (static_cast<dlimb_t>(a.m_limbs[1u]) << limb_bits)),
                      bd = static_cast<dlimb_t>(b.m_limbs[0u] + (static_cast<dlimb_t>(b.m_limbs[1u]) << limb_bits));
        const dlimb_t qd = static_cast<dlimb_t>(ad / bd), rd = static_cast<dlimb_t>(ad % bd);
        // Convert back to array of limb_t.
        q.m_limbs[0u] = static_cast<limb_t>(qd);
        q.m_limbs[1u] = static_cast<limb_t>(qd >> limb_bits);
        q.clear_extra_bits();
        q._mp_size = q.calculate_n_limbs();
        r.m_limbs[0u] = static_cast<limb_t>(rd);
        r.m_limbs[1u] = static_cast<limb_t>(rd >> limb_bits);
        r.clear_extra_bits();
        r._mp_size = r.calculate_n_limbs();
        // The sign of the remainder is the same as the numerator.
        if (!signa) {
            r.negate();
        }
        // The sign of the quotient is the sign of a/b.
        if (signa != signb) {
            q.negate();
        }
    }
    // Compute the number of bits used in the representation of the integer.
    // It will always return at least 1.
    // NOTE: of course, this can be greatly improved performance-wise. See
    // some ideas here:
    // https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    // This routine is not currently used in any performance-critical code,
    // but in case this changes we need to improve the implementation.
    limb_t bits_size() const
    {
        using size_type = typename limbs_type::size_type;
        const auto asize = abs_size();
        if (asize == 0) {
            return 1u;
        }
        const auto idx = static_cast<size_type>(asize - 1);
        limb_t size = static_cast<limb_t>(limb_bits * idx), limb = m_limbs[idx];
        while (limb != 0u) {
            ++size;
            limb = static_cast<limb_t>(limb >> 1u);
        }
        return size;
    }
    limb_t test_bit(const limb_t &idx) const
    {
        using size_type = typename limbs_type::size_type;
        piranha_assert(idx < limb_bits * 2u);
        const auto quot = static_cast<limb_t>(idx / limb_bits), rem = static_cast<limb_t>(idx % limb_bits);
        return (static_cast<limb_t>(m_limbs[static_cast<size_type>(quot)] & static_cast<limb_t>(limb_t(1u) << rem))
                != 0u);
    }
    // Some metaprogramming to make sure we can compute the hash safely. A bit of paranoid defensive programming.
    struct hash_checks {
        // Total number of bits that can be stored. We know already this operation is safe.
        static const limb_t tot_bits = static_cast<limb_t>(limb_bits * 2u);
        static const unsigned nbits_size_t = static_cast<unsigned>(std::numeric_limits<std::size_t>::digits);
        static const limb_t q = static_cast<limb_t>(tot_bits / nbits_size_t);
        static const limb_t r = static_cast<limb_t>(tot_bits % nbits_size_t);
        // Max number of size_t elements that can be extracted.
        static const limb_t max_n_size_t = q + limb_t(r != 0u);
        static const bool value =
            // Compute tot_nbits as unsigned.
            (tot_bits < std::numeric_limits<unsigned>::max()) &&
            // Convert n_size_t to std::size_t for use as function param in read_uint.
            (max_n_size_t < std::numeric_limits<std::size_t>::max()) &&
            // Multiplication in read_uint that could overflow. max_n_size_t - 1u is the maximum
            // value of the index param.
            (limb_t(max_n_size_t - 1u) < std::size_t(std::numeric_limits<std::size_t>::max() / nbits_size_t));
    };
    std::size_t hash() const
    {
        static_assert(hash_checks::value, "Overflow error.");
        // Hash of zero is zero.
        if (_mp_size == 0) {
            return 0;
        }
        // Use sign as seed value.
        mpz_size_t asize = _mp_size;
        std::size_t retval = 1u;
        if (_mp_size < 0) {
            asize = -asize;
            retval = static_cast<std::size_t>(-1);
        }
        // Determine the number of std::size_t to extract.
        const unsigned nbits_size_t = static_cast<unsigned>(std::numeric_limits<std::size_t>::digits),
                       tot_nbits = static_cast<unsigned>(limb_bits * static_cast<unsigned>(asize)),
                       q = tot_nbits / nbits_size_t, r = tot_nbits % nbits_size_t, n_size_t = q + unsigned(r != 0u);
        for (unsigned i = 0u; i < n_size_t; ++i) {
            boost::hash_combine(
                retval, read_uint<std::size_t, total_bits - limb_bits>(&m_limbs[0u], 2u, static_cast<std::size_t>(i)));
        }
        return retval;
    }
    mpz_alloc_t _mp_alloc;
    mpz_size_t _mp_size;
    limbs_type m_limbs;
};

// Static init.
template <int NBits>
const typename static_integer<NBits>::limb_t static_integer<NBits>::limb_bits;

// Integer union.
template <int NBits>
union integer_union {
public:
    using s_storage = static_integer<NBits>;
    using d_storage = mpz_struct_t;
    static void move_ctor_mpz(mpz_struct_t &to, mpz_struct_t &from)
    {
        to._mp_alloc = from._mp_alloc;
        to._mp_size = from._mp_size;
        to._mp_d = from._mp_d;
    }
    integer_union() : m_st()
    {
    }
    integer_union(const integer_union &other)
    {
        if (other.is_static()) {
            ::new (static_cast<void *>(&m_st)) s_storage(other.g_st());
        } else {
            ::new (static_cast<void *>(&m_dy)) d_storage;
            ::mpz_init_set(&m_dy, &other.g_dy());
            piranha_assert(m_dy._mp_alloc >= 0);
        }
    }
    integer_union(integer_union &&other) noexcept
    {
        if (other.is_static()) {
            ::new (static_cast<void *>(&m_st)) s_storage(std::move(other.g_st()));
        } else {
            ::new (static_cast<void *>(&m_dy)) d_storage;
            move_ctor_mpz(m_dy, other.g_dy());
            // Downgrade the other to an empty static.
            other.g_dy().~d_storage();
            ::new (static_cast<void *>(&other.m_st)) s_storage();
        }
    }
    ~integer_union()
    {
        if (is_static()) {
            g_st().~s_storage();
        } else {
            destroy_dynamic();
        }
    }
    integer_union &operator=(const integer_union &other)
    {
        if (unlikely(this == &other)) {
            return *this;
        }
        const bool s1 = is_static(), s2 = other.is_static();
        if (s1 && s2) {
            g_st() = other.g_st();
        } else if (s1 && !s2) {
            // Destroy static.
            g_st().~s_storage();
            // Construct the dynamic struct.
            ::new (static_cast<void *>(&m_dy)) d_storage;
            // Init + assign the mpz.
            ::mpz_init_set(&m_dy, &other.g_dy());
            piranha_assert(m_dy._mp_alloc >= 0);
        } else if (!s1 && s2) {
            // Destroy the dynamic this.
            destroy_dynamic();
            // Init-copy the static from other.
            ::new (static_cast<void *>(&m_st)) s_storage(other.g_st());
        } else {
            ::mpz_set(&g_dy(), &other.g_dy());
        }
        return *this;
    }
    integer_union &operator=(integer_union &&other) noexcept
    {
        if (unlikely(this == &other)) {
            return *this;
        }
        const bool s1 = is_static(), s2 = other.is_static();
        if (s1 && s2) {
            g_st() = std::move(other.g_st());
        } else if (s1 && !s2) {
            // Destroy static.
            g_st().~s_storage();
            // Construct the dynamic struct.
            ::new (static_cast<void *>(&m_dy)) d_storage;
            move_ctor_mpz(m_dy, other.g_dy());
            // Downgrade the other to an empty static.
            other.g_dy().~d_storage();
            ::new (static_cast<void *>(&other.m_st)) s_storage();
        } else if (!s1 && s2) {
            // Same as copy assignment: destroy and copy-construct.
            destroy_dynamic();
            ::new (static_cast<void *>(&m_st)) s_storage(other.g_st());
        } else {
            // Swap with other.
            ::mpz_swap(&g_dy(), &other.g_dy());
        }
        return *this;
    }
    bool is_static() const
    {
        return m_st._mp_alloc == mpz_alloc_t(-1);
    }
    static bool fits_in_static(const mpz_struct_t &mpz)
    {
        // NOTE: sizeinbase returns the index of the highest bit *counting from 1* (like a logarithm).
        return (::mpz_sizeinbase(&mpz, 2) <= s_storage::limb_bits * 2u);
    }
    void destroy_dynamic()
    {
        piranha_assert(!is_static());
        piranha_assert(g_dy()._mp_alloc >= 0);
        piranha_assert(g_dy()._mp_d != nullptr);
        ::mpz_clear(&g_dy());
        m_dy.~d_storage();
    }
    void promote()
    {
        piranha_assert(is_static());
        // Construct an mpz from the static.
        mpz_struct_t tmp_mpz;
        auto v = g_st().get_mpz_view();
        ::mpz_init_set(&tmp_mpz, v);
        // Destroy static.
        g_st().~s_storage();
        // Construct the dynamic struct.
        ::new (static_cast<void *>(&m_dy)) d_storage;
        move_ctor_mpz(m_dy, tmp_mpz);
        // No need to do anything, as move_ctor_mpz() transfers
        // ownership to m_dy.
    }
    // Getters for st and dy.
    const s_storage &g_st() const
    {
        piranha_assert(is_static());
        return m_st;
    }
    s_storage &g_st()
    {
        piranha_assert(is_static());
        return m_st;
    }
    const d_storage &g_dy() const
    {
        piranha_assert(!is_static());
        return m_dy;
    }
    d_storage &g_dy()
    {
        piranha_assert(!is_static());
        return m_dy;
    }

private:
    s_storage m_st;
    d_storage m_dy;
};

// Detect type interoperable with mp_integer.
template <typename T>
struct is_mp_integer_interoperable_type {
    static const bool value = std::is_floating_point<T>::value || std::is_integral<T>::value;
};
}

/// Multiple precision integer class.
/**
 * This class is a wrapper around the GMP arbitrary precision \p mpz_t type, and it can represent integer numbers of
 * arbitrary size
 * (i.e., the range is limited only by the available memory).
 *
 * As an optimisation, this class will store in static internal storage a fixed number of digits before resorting to
 * dynamic
 * memory allocation. The internal storage consists of two limbs of size \p NBits bits, for a total of <tt>2*NBits</tt>
 * bits
 * of static storage. The possible values for \p NBits, supported on all platforms, are 8, 16, and 32.
 * A value of 64 is supported on some platforms. The special
 * default value of 0 is used to automatically select the optimal \p NBits value on the current platform.
 *
 * ## Interoperability with other types ##
 *
 * Full interoperability with all integral and floating-point C++ types is provided.
 *
 * Every function interacting with floating-point types will check that the floating-point values are not
 * non-finite: in case of infinities or NaNs, an <tt>std::invalid_argument</tt> exception will be thrown.
 * It should be noted that interoperability with floating-point types is provided for convenience, and it should
 * not be relied upon in case exact results are required (e.g., the conversion of a large integer to a floating-point
 * value
 * might not be exact).
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the strong exception safety guarantee for all operations. In case of memory allocation errors by
 * GMP routines,
 * the program will terminate.
 *
 * ## Move semantics ##
 *
 * Move construction and move assignment will leave the moved-from object in an unspecified but valid state.
 *
 * ## Implementation details ##
 *
 * This class uses, for certain routines, the internal interface of GMP integers, which is not guaranteed to be stable
 * across different versions. GMP versions 4.x, 5.x and 6.x are explicitly supported by this class.
 *
 * ## Serialization ##
 *
 * This class supports serialization.
 *
 * @see http://gmplib.org/
 */
/*
 * NOTES:
 * - performance improvements:
 *   - it seems like for a bunch of operations we do not need GMP anymore (e.g., conversion to float),
 *     we can use mp_integer directly - this could be a performance improvement;
 *   - avoid going through mpz for print to stream.
 * - consider if and how to implement demoting. It looks it could be useful in certain cases, for instance when we
 *   rely on GMP routines (we should demote back to static if possible in those cases). For the elementary operations,
 *   it is less clear: addition, subtraction and division could in principle be considered for demotion. But, OTOH
 *   GMP never scales back the allocated memory and, for instance, also the std:: containers do not normally reduce
 *   their sizes. There might be some lesson in there;
 * - understand the performance implications of implementing the binary operator as += and copy. Might be that creating
 * an
 *   empty retval and then filling it with mpz_add() or a similar free function is more efficient. See how it is done
 *   in Arbpp for instance. This should matter much more for mp_integer and not mp_rational, as there we always
 *   have the canonicalisation to do anyway.
 *   Note that now we have the ternary arithmetic functions implemented, which could be used for this.
 * - apparently, the mpfr devs are considering adding an fma-like functions that computes ab +/- cd. It seems like this
 * would
 *   be useful for both rational and complex numbers, maybe we could implement it here as well.
 * - the speed of conversion to floating-point might matter in series evaluation. Consider what happens when evaluating
 *   cos(x) in which x an mp_integer (passed in from Python, for instance). If we overload cos() to produce a double for
 * int argument,
 *   then we need to convert x to double and then compute cos(x). Note that for 1-limb numbers we actually could do
 * directly
 *   the conversion to double of the limb, this should be quite fast.
 * - when converting to/from Python we can speed up operations by trying casting around to hardware integers, if range
 * is enough.
 * - use a unified shortcut for the possible optimisation when the two limb type coincide (e.g., same_limbs_type = true
 * constexpr).
 * - the conversion operator to C++ integral types could use the same optimisation as the constructor from integral
 * types (e.g,
 *   attempt direct conversion if we have only 1 limb) -> this is now done. We should consider piggybacking on the GMP
 * functions
 *   when possible, and use our generic implementation only as last resort.
 * - more in general, for conversions to/from other types we should consider operating directly with limbs instead of
 * bit-by-bit
 *   for increased performance. We should be able to do this with the new shift operators.
 * - there is some out-of-range conversion handling which is not hit by the test cases, need to proper test it.
 * - fits_in_static() might be made obsolete by the new mpz_t ctor. fits_in_static() is more accurate, but potentially
 *   slower and prone to overflow. See if it makes sense to replace it.
 * - use the GMP facilities via mpz_view and/or the mpz_t constructor when interacting with float and double? Maybe pass
 * through a real
 *   for interaction with long doubles? This might need a thread local mpz_t/real/mpfr_t in order to avoid having to
 * allocate at each
 *   construction, but for thread local we have the usual issue on OSX.
 * - if GMP ever adopts sane behaviour for memory errors or if we ever move away from it, we probably need to review the
 * exception behaviour,
 *   and possibly re-implement a bunch of things. For instance, now the copy-assignment operator is defaulted in
 * mp_rational, but if exceptions
 *   are allowed then we need to change the implementation to the copy+move idiom.
 */
template <int NBits = 0>
class mp_integer
{
    // Make friend with debugging class, mp_rational and real.
    template <typename>
    friend class debug_access;
    template <int>
    friend class mp_rational;
    friend class real;
    // Import the interoperable types detector.
    template <typename T>
    using is_interoperable_type = detail::is_mp_integer_interoperable_type<T>;
    // Enabler for generic ctor.
    template <typename T>
    using generic_ctor_enabler = typename std::enable_if<is_interoperable_type<T>::value, int>::type;
    // Cast enabler.
    template <typename T>
    using cast_enabler = generic_ctor_enabler<T>;
    // Enabler for in-place arithmetic operations with interop on the left.
    template <typename T>
    using generic_in_place_enabler =
        typename std::enable_if<is_interoperable_type<T>::value && !std::is_const<T>::value, int>::type;
    // Enabler for in-place mod with interop on the left.
    template <typename T>
    using generic_in_place_mod_enabler = typename std::
        enable_if<is_interoperable_type<T>::value && !std::is_const<T>::value && std::is_integral<T>::value, int>::type;
    // Enabler for in-place shifts with interop on the left.
    template <typename T>
    using generic_in_place_shift_enabler = typename std::
        enable_if<is_interoperable_type<T>::value && !std::is_const<T>::value && std::is_integral<T>::value, int>::type;
    template <typename Float>
    void construct_from_interoperable(const Float &x,
                                      typename std::enable_if<std::is_floating_point<Float>::value>::type * = nullptr)
    {
        if (unlikely(!std::isfinite(x))) {
            piranha_throw(std::invalid_argument, "cannot construct an integer from a non-finite floating-point number");
        }
        if (x == Float(0)) {
            return;
        }
        Float abs_x = std::abs(x);
        const unsigned radix = static_cast<unsigned>(std::numeric_limits<Float>::radix);
        detail::mpz_raii m, tmp;
        int exp = std::ilogb(abs_x);
        while (exp >= 0) {
            ::mpz_ui_pow_ui(&tmp.m_mpz, radix, static_cast<unsigned>(exp));
            ::mpz_add(&m.m_mpz, &m.m_mpz, &tmp.m_mpz);
            const Float ftmp = std::scalbn(Float(1), exp);
            if (unlikely(ftmp == HUGE_VAL)) {
                piranha_throw(std::invalid_argument, "output of std::scalbn is HUGE_VAL");
            }
            abs_x -= ftmp;
            // NOTE: if the float is an exact integer, we eventually
            // get to abs_x == 0, in which case we have to prevent the call to ilogb below.
            if (unlikely(abs_x == Float(0))) {
                break;
            }
            // NOTE: in principle, here ilogb could return funky values if something goes wrong
            // or the initial call to ilogb gave an undefined result for some reason:
            // http://en.cppreference.com/w/cpp/numeric/math/ilogb
            exp = std::ilogb(abs_x);
            if (unlikely(exp == INT_MAX || exp == FP_ILOGBNAN)) {
                piranha_throw(std::invalid_argument, "error calling std::ilogb");
            }
        }
        if (m_int.fits_in_static(m.m_mpz)) {
            using limb_t = typename detail::integer_union<NBits>::s_storage::limb_t;
            const auto size2 = ::mpz_sizeinbase(&m.m_mpz, 2);
            for (::mp_bitcnt_t i = 0u; i < size2; ++i) {
                if (::mpz_tstbit(&m.m_mpz, i)) {
                    m_int.g_st().set_bit(static_cast<limb_t>(i));
                }
            }
            if (x < Float(0)) {
                m_int.g_st().negate();
            }
        } else {
            m_int.promote();
            ::mpz_swap(&m.m_mpz, &m_int.g_dy());
            if (x < Float(0)) {
                ::mpz_neg(&m_int.g_dy(), &m_int.g_dy());
            }
        }
    }
    template <typename Integer>
    void construct_from_interoperable(const Integer &n_orig,
                                      typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        // Try to use the static algorithm first.
        try {
            m_int.g_st().construct_from_integral(n_orig);
            return;
        } catch (const std::overflow_error &) {
            // Check that everything was cleaned properly, in case the exception was triggered,
            // and continue.
            piranha_assert(m_int.g_st()._mp_alloc == detail::mpz_alloc_t(-1));
            piranha_assert(m_int.g_st()._mp_size == 0);
            piranha_assert(m_int.g_st().m_limbs[0u] == 0u);
            piranha_assert(m_int.g_st().m_limbs[1u] == 0u);
        }
        // Go through a temp mpz for the construction.
        Integer n = n_orig;
        detail::mpz_raii m;
        ::mp_bitcnt_t bit_idx = 0;
        while (n != Integer(0)) {
            Integer div = static_cast<Integer>(n / Integer(2)), rem = static_cast<Integer>(n % Integer(2));
            if (rem != Integer(0)) {
                ::mpz_setbit(&m.m_mpz, bit_idx);
            }
            if (unlikely(bit_idx == std::numeric_limits<::mp_bitcnt_t>::max())) {
                piranha_throw(std::invalid_argument, "overflow in the construction from integral type");
            }
            ++bit_idx;
            n = div;
        }
        // Promote the current static to dynamic storage.
        m_int.promote();
        // Swap in the temp mpz.
        ::mpz_swap(&m.m_mpz, &m_int.g_dy());
        // Fix the sign as needed.
        if (n_orig <= Integer(0)) {
            ::mpz_neg(&m_int.g_dy(), &m_int.g_dy());
        }
    }
    // Special casing for bool.
    void construct_from_interoperable(bool v)
    {
        if (v) {
            m_int.g_st()._mp_size = 1;
            m_int.g_st().m_limbs[0u] = 1u;
        }
    }
    static void validate_string(const char *str, const std::size_t &size)
    {
        if (!size) {
            piranha_throw(std::invalid_argument, "invalid string input for integer type");
        }
        const std::size_t has_minus = (str[0] == '-'), signed_size = static_cast<std::size_t>(size - has_minus);
        if (!signed_size) {
            piranha_throw(std::invalid_argument, "invalid string input for integer type");
        }
        // A number starting with zero cannot be multi-digit and cannot have a leading minus sign (no '-0' allowed).
        if (str[has_minus] == '0' && (signed_size > 1u || has_minus)) {
            piranha_throw(std::invalid_argument, "invalid string input for integer type");
        }
        // Check that each character is a digit.
        std::for_each(str + has_minus, str + size, [](char c) {
            if (!detail::is_digit(c)) {
                piranha_throw(std::invalid_argument, "invalid string input for integer type");
            }
        });
    }
    void construct_from_string(const char *str)
    {
        // NOTE: it seems to be ok to call strlen on a char pointer obtained from std::string::c_str()
        // (as we do in the constructor from std::string). The output of c_str() is guaranteed
        // to be NULL terminated, and if the string is empty, this will still work (21.4.7 and around).
        validate_string(str, std::strlen(str));
        // String is OK.
        detail::mpz_raii m;
        // Use set() as m is already inited.
        const int retval = ::mpz_set_str(&m.m_mpz, str, 10);
        if (retval == -1) {
            piranha_throw(std::invalid_argument, "invalid string input for integer type");
        }
        piranha_assert(retval == 0);
        const bool negate = (mpz_sgn(&m.m_mpz) == -1);
        if (negate) {
            ::mpz_neg(&m.m_mpz, &m.m_mpz);
        }
        if (m_int.fits_in_static(m.m_mpz)) {
            using limb_t = typename detail::integer_union<NBits>::s_storage::limb_t;
            const auto size2 = ::mpz_sizeinbase(&m.m_mpz, 2);
            for (::mp_bitcnt_t i = 0u; i < size2; ++i) {
                if (::mpz_tstbit(&m.m_mpz, i)) {
                    m_int.g_st().set_bit(static_cast<limb_t>(i));
                }
            }
            if (negate) {
                m_int.g_st().negate();
            }
        } else {
            m_int.promote();
            ::mpz_swap(&m.m_mpz, &m_int.g_dy());
            if (negate) {
                ::mpz_neg(&m_int.g_dy(), &m_int.g_dy());
            }
        }
    }
    // Conversion to integral types.
    template <typename T>
    static void check_mult2(const T &n, typename std::enable_if<std::is_signed<T>::value>::type * = nullptr)
    {
        if (n < std::numeric_limits<T>::min() / T(2) || n > std::numeric_limits<T>::max() / T(2)) {
            piranha_throw(std::overflow_error, "overflow in conversion to integral type");
        }
    }
    template <typename T>
    static void check_mult2(const T &n, typename std::enable_if<!std::is_signed<T>::value>::type * = nullptr)
    {
        if (n > std::numeric_limits<T>::max() / T(2)) {
            piranha_throw(std::overflow_error, "overflow in conversion to integral type");
        }
    }
    // Conversion of 1-limb statics. Can fail depending on T. The flag param is initially
    // false and set to true only if the operation succeeds.
    template <typename T, typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
    void attempt_fast_1_limb_conversion(T &retval, bool &flag) const
    {
        // Attempt something only if there's only 1 limb and the number is positive.
        // NOTE: the zero case is handled in the upper function.
        if (m_int.g_st()._mp_size == 1) {
            try {
                retval = boost::numeric_cast<T>(m_int.g_st().m_limbs[0u]);
                flag = true;
            } catch (...) {
                piranha_throw(std::overflow_error, "overflow in conversion to integral type");
            }
        }
    }
    template <typename T, typename std::enable_if<std::is_signed<T>::value, int>::type = 0>
    void attempt_fast_1_limb_conversion(T &retval, bool &flag) const
    {
        // Like above for the 1 positive limb case.
        if (m_int.g_st()._mp_size == 1) {
            try {
                retval = boost::numeric_cast<T>(m_int.g_st().m_limbs[0u]);
                flag = true;
            } catch (...) {
                piranha_throw(std::overflow_error, "overflow in conversion to integral type");
            }
        }
        // 1 negative limb case.
        if (m_int.g_st()._mp_size == -1) {
            try {
                // This will first be the negative of the final value, if computable.
                retval = boost::numeric_cast<T>(m_int.g_st().m_limbs[0u]);
                piranha_assert(retval > 0);
                // Now check if we can negate it.
                if (retval <= detail::safe_abs_sint<T>::value) {
                    retval = static_cast<T>(-retval);
                    flag = true;
                }
            } catch (...) {
                // In this case we don't want to do anything as the conversion might still
                // be possible in principle, and we go through the general case.
            }
        }
    }
    template <typename T>
    T convert_to_impl(
        typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type * = nullptr) const
    {
        // Special case for zero first.
        const int s = sign();
        if (s == 0) {
            return T(0);
        }
        // Next try the fast static variant.
        if (m_int.is_static()) {
            bool flag = false;
            T retval(0);
            attempt_fast_1_limb_conversion(retval, flag);
            if (flag) {
                return retval;
            }
        }
        const bool negative = s < 0;
        // We cannot convert to unsigned type if this is negative.
        if (std::is_unsigned<T>::value && negative) {
            piranha_throw(std::overflow_error, "overflow in conversion to integral type");
        }
        T retval(0), tmp(static_cast<T>(negative ? -1 : 1));
        if (m_int.is_static()) {
            using limb_t = typename detail::integer_union<NBits>::s_storage::limb_t;
            const limb_t bits_size = m_int.g_st().bits_size();
            piranha_assert(bits_size != 0u);
            for (limb_t i = 0u; i < bits_size; ++i) {
                if (i != 0u) {
                    check_mult2(tmp);
                    tmp = static_cast<T>(tmp * T(2));
                }
                if (m_int.g_st().test_bit(i)) {
                    if (negative && retval < std::numeric_limits<T>::min() - tmp) {
                        piranha_throw(std::overflow_error, "overflow in conversion to integral type");
                    } else if (!negative && retval > std::numeric_limits<T>::max() - tmp) {
                        piranha_throw(std::overflow_error, "overflow in conversion to integral type");
                    }
                    retval = static_cast<T>(retval + tmp);
                }
            }
        } else {
            // NOTE: copy here, it's not the fastest way but it should be safer.
            detail::mpz_raii tmp_mpz;
            ::mpz_set(&tmp_mpz.m_mpz, &m_int.g_dy());
            // Adjust the sign as needed, in order to use the test bit function below.
            if (mpz_sgn(&tmp_mpz.m_mpz) == -1) {
                ::mpz_neg(&tmp_mpz.m_mpz, &tmp_mpz.m_mpz);
            }
            const std::size_t bits_size = ::mpz_sizeinbase(&tmp_mpz.m_mpz, 2);
            piranha_assert(bits_size != 0u);
            for (std::size_t i = 0u; i < bits_size; ++i) {
                if (i != 0u) {
                    check_mult2(tmp);
                    tmp = static_cast<T>(tmp * T(2));
                }
                ::mp_bitcnt_t bit_idx;
                try {
                    bit_idx = boost::numeric_cast<::mp_bitcnt_t>(i);
                } catch (...) {
                    piranha_throw(std::overflow_error, "overflow in conversion to integral type");
                }
                if (::mpz_tstbit(&tmp_mpz.m_mpz, bit_idx)) {
                    if (negative && retval < std::numeric_limits<T>::min() - tmp) {
                        piranha_throw(std::overflow_error, "overflow in conversion to integral type");
                    } else if (!negative && retval > std::numeric_limits<T>::max() - tmp) {
                        piranha_throw(std::overflow_error, "overflow in conversion to integral type");
                    }
                    retval = static_cast<T>(retval + tmp);
                }
            }
        }
        return retval;
    }
    // Special casing for bool.
    template <typename T>
    T convert_to_impl(typename std::enable_if<std::is_same<T, bool>::value>::type * = nullptr) const
    {
        return sign() != 0;
    }
    // Convert to floating-point.
    template <typename T>
    T convert_to_impl(typename std::enable_if<std::is_same<double, T>::value>::type * = nullptr) const
    {
        // For double, we just use mpz_get_d().
        auto v = get_mpz_view();
        return ::mpz_get_d(v);
    }
    template <typename T>
    T convert_to_impl(typename std::enable_if<std::is_same<float, T>::value>::type * = nullptr) const
    {
        // Convert to double, then cast back to float.
        return static_cast<float>(convert_to_impl<double>());
    }
    template <typename T>
    T convert_to_impl(typename std::enable_if<std::is_same<long double, T>::value>::type * = nullptr) const
    {
        // For long double, create a temporary mpfr, init it with this, then extract the long double.
        // NOTE: here we should really use a thread local mpfr_t wrapped into a RAII holder.
        // Overflow check for the operation below.
        static_assert(std::numeric_limits<long double>::digits10 < (std::numeric_limits<int>::max() - 10) / 3,
                      "Overflow error.");
        // This is the number of digits in base 2 needed to represent exactly any long double.
        // The exact number should be  + 1 instead of + 10, but let's be conservative.
        // http://en.cppreference.com/w/cpp/types/numeric_limits/digits10
        constexpr int d2 = std::numeric_limits<long double>::digits10 * 3 + 10;
        const auto prec = boost::numeric_cast<::mpfr_prec_t>(d2);
        auto v = get_mpz_view();
        // All the rest will be noexcept.
        ::mpfr_t tmp;
        ::mpfr_init2(tmp, prec);
        ::mpfr_set_z(tmp, v, MPFR_RNDN);
        long double retval = ::mpfr_get_ld(tmp, MPFR_RNDN);
        ::mpfr_clear(tmp);
        return retval;
    }
    // In-place add.
    template <bool AddOrSub>
    mp_integer &in_place_add_or_sub(const mp_integer &other)
    {
        bool s1 = is_static(), s2 = other.is_static();
        if (s1 && s2) {
            // Attempt the static add/sub.
            // NOTE: the static add/sub will try to do the operation, if it fails 1
            // will be returned. The operation is safe, and m_int.g_st() will be untouched
            // in case of problems.
            int status;
            if (AddOrSub) {
                status = m_int.g_st().add(m_int.g_st(), m_int.g_st(), other.m_int.g_st());
            } else {
                status = m_int.g_st().sub(m_int.g_st(), m_int.g_st(), other.m_int.g_st());
            }
            if (likely(!status)) {
                return *this;
            }
        }
        // Promote as needed, we need GMP types on both sides.
        if (s1) {
            m_int.promote();
            // NOTE: refresh the value of s2 in case other coincided with this.
            s2 = other.is_static();
        }
        if (s2) {
            auto v = other.m_int.g_st().get_mpz_view();
            if (AddOrSub) {
                ::mpz_add(&m_int.g_dy(), &m_int.g_dy(), v);
            } else {
                ::mpz_sub(&m_int.g_dy(), &m_int.g_dy(), v);
            }
        } else {
            if (AddOrSub) {
                ::mpz_add(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
            } else {
                ::mpz_sub(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
            }
        }
        return *this;
    }
    mp_integer &in_place_add(const mp_integer &other)
    {
        return in_place_add_or_sub<true>(other);
    }
    template <typename T>
    mp_integer &in_place_add(const T &other, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
    {
        return in_place_add(mp_integer(other));
    }
    template <typename T>
    mp_integer &in_place_add(const T &other,
                             typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (*this = static_cast<T>(*this) + other);
    }
    // Binary add.
    template <typename T, typename U>
    static mp_integer binary_plus(const T &n1, const U &n2,
                                  typename std::enable_if<std::is_same<T, mp_integer>::value
                                                          && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval += n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_plus(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        // NOTE: for binary ops, let's do first the conversion to mp_integer and then
        // use the mp_integer vs mp_integer operator.
        mp_integer retval(n2);
        retval += n1;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_plus(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval += n2;
        return retval;
    }
    template <typename T>
    static T binary_plus(const mp_integer &n, const T &x,
                         typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (static_cast<T>(n) + x);
    }
    template <typename T>
    static T binary_plus(const T &x, const mp_integer &n,
                         typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return binary_plus(n, x);
    }
    // Subtraction.
    mp_integer &in_place_sub(const mp_integer &other)
    {
        return in_place_add_or_sub<false>(other);
    }
    template <typename T>
    mp_integer &in_place_sub(const T &other, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
    {
        return in_place_sub(mp_integer(other));
    }
    template <typename T>
    mp_integer &in_place_sub(const T &other,
                             typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (*this = static_cast<T>(*this) - other);
    }
    // Binary subtraction.
    template <typename T, typename U>
    static mp_integer binary_subtract(const T &n1, const U &n2,
                                      typename std::enable_if<std::is_same<T, mp_integer>::value
                                                              && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n2);
        retval -= n1;
        retval.negate();
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_subtract(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        mp_integer retval(n2);
        retval -= n1;
        retval.negate();
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_subtract(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval -= n2;
        return retval;
    }
    template <typename T>
    static T binary_subtract(const mp_integer &n, const T &x,
                             typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (static_cast<T>(n) - x);
    }
    template <typename T>
    static T binary_subtract(const T &x, const mp_integer &n,
                             typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return -binary_subtract(n, x);
    }
    // Multiplication.
    mp_integer &in_place_mul(const mp_integer &other)
    {
        bool s1 = is_static(), s2 = other.is_static();
        if (s1 && s2) {
            // Attempt the static mul.
            if (likely(!m_int.g_st().mul(m_int.g_st(), m_int.g_st(), other.m_int.g_st()))) {
                return *this;
            }
        }
        // Promote as needed, we need GMP types on both sides.
        if (s1) {
            m_int.promote();
            s2 = other.is_static();
        }
        if (s2) {
            auto v = other.m_int.g_st().get_mpz_view();
            ::mpz_mul(&m_int.g_dy(), &m_int.g_dy(), v);
        } else {
            ::mpz_mul(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
        }
        return *this;
    }
    template <typename T>
    mp_integer &in_place_mul(const T &other, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
    {
        return in_place_mul(mp_integer(other));
    }
    template <typename T>
    mp_integer &in_place_mul(const T &other,
                             typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (*this = static_cast<T>(*this) * other);
    }
    template <typename T, typename U>
    static mp_integer binary_mul(const T &n1, const U &n2,
                                 typename std::enable_if<std::is_same<T, mp_integer>::value
                                                         && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n2);
        retval *= n1;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_mul(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        mp_integer retval(n2);
        retval *= n1;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_mul(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval *= n2;
        return retval;
    }
    template <typename T>
    static T binary_mul(const mp_integer &n, const T &x,
                        typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (static_cast<T>(n) * x);
    }
    template <typename T>
    static T binary_mul(const T &x, const mp_integer &n,
                        typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return binary_mul(n, x);
    }
    // Division.
    mp_integer &in_place_div(const mp_integer &other)
    {
        const bool s1 = is_static(), s2 = other.is_static();
        if (s1 && s2) {
            mp_integer r;
            m_int.g_st().div(m_int.g_st(), r.m_int.g_st(), m_int.g_st(), other.m_int.g_st());
        } else if (s1 && !s2) {
            // Promote this.
            m_int.promote();
            ::mpz_tdiv_q(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
        } else if (!s1 && s2) {
            auto v = other.m_int.g_st().get_mpz_view();
            ::mpz_tdiv_q(&m_int.g_dy(), &m_int.g_dy(), v);
        } else {
            ::mpz_tdiv_q(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
        }
        return *this;
    }
    template <typename T>
    mp_integer &in_place_div(const T &other, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
    {
        return in_place_div(mp_integer(other));
    }
    template <typename T>
    mp_integer &in_place_div(const T &other,
                             typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (*this = static_cast<T>(*this) / other);
    }
    template <typename T, typename U>
    static mp_integer binary_div(const T &n1, const U &n2,
                                 typename std::enable_if<std::is_same<T, mp_integer>::value
                                                         && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval /= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_div(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval /= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_div(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval /= n2;
        return retval;
    }
    template <typename T>
    static T binary_div(const mp_integer &n, const T &x,
                        typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (static_cast<T>(n) / x);
    }
    template <typename T>
    static T binary_div(const T &x, const mp_integer &n,
                        typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr)
    {
        return (x / static_cast<T>(n));
    }
    // Modulo.
    mp_integer &in_place_mod(const mp_integer &other)
    {
        const bool s1 = is_static(), s2 = other.is_static();
        if (s1 && s2) {
            mp_integer q;
            m_int.g_st().div(q.m_int.g_st(), m_int.g_st(), m_int.g_st(), other.m_int.g_st());
        } else if (s1 && !s2) {
            // Promote this.
            m_int.promote();
            ::mpz_tdiv_r(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
        } else if (!s1 && s2) {
            auto v = other.m_int.g_st().get_mpz_view();
            ::mpz_tdiv_r(&m_int.g_dy(), &m_int.g_dy(), v);
        } else {
            ::mpz_tdiv_r(&m_int.g_dy(), &m_int.g_dy(), &other.m_int.g_dy());
        }
        return *this;
    }
    template <typename T>
    mp_integer &in_place_mod(const T &other, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
    {
        return in_place_mod(mp_integer(other));
    }
    template <typename T, typename U>
    static mp_integer binary_mod(const T &n1, const U &n2,
                                 typename std::enable_if<std::is_same<T, mp_integer>::value
                                                         && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval %= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_mod(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval %= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_mod(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval %= n2;
        return retval;
    }
    // Left-Shift
    mp_integer &in_place_lshift_mp_bitcnt_t(::mp_bitcnt_t other)
    {
        if (is_static()) {
            int status = m_int.g_st().lshift(other);
            if (status == 0) {
                return *this;
            }
            // Promote this.
            m_int.promote();
        }
        ::mpz_mul_2exp(&m_int.g_dy(), &m_int.g_dy(), other);
        return *this;
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    mp_integer &in_place_lshift(const T &other)
    {
        try {
            return in_place_lshift_mp_bitcnt_t(boost::numeric_cast<::mp_bitcnt_t>(other));
        } catch (const boost::numeric::bad_numeric_cast &) {
            piranha_throw(std::invalid_argument, "invalid argument for left bit shifting");
        }
    }
    mp_integer &in_place_lshift(const mp_integer &other)
    {
        try {
            return in_place_lshift_mp_bitcnt_t(static_cast<::mp_bitcnt_t>(other));
        } catch (const std::overflow_error &) {
            piranha_throw(std::invalid_argument, "invalid argument for left bit shifting");
        }
    }
    template <typename T, typename U>
    static mp_integer binary_lshift(const T &n1, const U &n2,
                                    typename std::enable_if<std::is_same<T, mp_integer>::value
                                                            && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval <<= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_lshift(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval <<= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_lshift(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval <<= n2;
        return retval;
    }
    // Right-Shift
    mp_integer &in_place_rshift_mp_bitcnt_t(::mp_bitcnt_t other)
    {
        if (is_static()) {
            m_int.g_st().rshift(other);
        } else {
            ::mpz_tdiv_q_2exp(&m_int.g_dy(), &m_int.g_dy(), other);
        }
        return *this;
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    mp_integer &in_place_rshift(const T &other)
    {
        try {
            return in_place_rshift_mp_bitcnt_t(boost::numeric_cast<::mp_bitcnt_t>(other));
        } catch (const boost::numeric::bad_numeric_cast &) {
            piranha_throw(std::invalid_argument, "invalid argument for right bit shifting");
        }
    }
    mp_integer &in_place_rshift(const mp_integer &other)
    {
        try {
            return in_place_rshift_mp_bitcnt_t(static_cast<::mp_bitcnt_t>(other));
        } catch (const std::overflow_error &) {
            piranha_throw(std::invalid_argument, "invalid argument for right bit shifting");
        }
    }
    template <typename T, typename U>
    static mp_integer binary_rshift(const T &n1, const U &n2,
                                    typename std::enable_if<std::is_same<T, mp_integer>::value
                                                            && std::is_same<U, mp_integer>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval >>= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_rshift(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<T, mp_integer>::value && std::is_integral<U>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval >>= n2;
        return retval;
    }
    template <typename T, typename U>
    static mp_integer binary_rshift(
        const T &n1, const U &n2,
        typename std::enable_if<std::is_same<U, mp_integer>::value && std::is_integral<T>::value>::type * = nullptr)
    {
        mp_integer retval(n1);
        retval >>= n2;
        return retval;
    }
    // Comparison.
    static bool binary_equality(const mp_integer &n1, const mp_integer &n2)
    {
        const bool s1 = n1.is_static(), s2 = n2.is_static();
        if (s1 && s2) {
            return (n1.m_int.g_st() == n2.m_int.g_st());
        } else if (s1 && !s2) {
            auto v1 = n1.m_int.g_st().get_mpz_view();
            return ::mpz_cmp(v1, &n2.m_int.g_dy()) == 0;
        } else if (!s1 && s2) {
            auto v2 = n2.m_int.g_st().get_mpz_view();
            return ::mpz_cmp(v2, &n1.m_int.g_dy()) == 0;
        } else {
            return ::mpz_cmp(&n1.m_int.g_dy(), &n2.m_int.g_dy()) == 0;
        }
    }
    template <typename Integer>
    static bool binary_equality(const mp_integer &n1, const Integer &n2,
                                typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        return n1 == mp_integer(n2);
    }
    template <typename Integer>
    static bool binary_equality(const Integer &n1, const mp_integer &n2,
                                typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        return binary_equality(n2, n1);
    }
    template <typename FloatingPoint>
    static bool binary_equality(const mp_integer &n, const FloatingPoint &x,
                                typename std::enable_if<std::is_floating_point<FloatingPoint>::value>::type * = nullptr)
    {
        return static_cast<FloatingPoint>(n) == x;
    }
    template <typename FloatingPoint>
    static bool binary_equality(const FloatingPoint &x, const mp_integer &n,
                                typename std::enable_if<std::is_floating_point<FloatingPoint>::value>::type * = nullptr)
    {
        return binary_equality(n, x);
    }
    static bool binary_less_than(const mp_integer &n1, const mp_integer &n2)
    {
        const bool s1 = n1.is_static(), s2 = n2.is_static();
        if (s1 && s2) {
            return (n1.m_int.g_st() < n2.m_int.g_st());
        } else if (s1 && !s2) {
            auto v1 = n1.m_int.g_st().get_mpz_view();
            return ::mpz_cmp(v1, &n2.m_int.g_dy()) < 0;
        } else if (!s1 && s2) {
            auto v2 = n2.m_int.g_st().get_mpz_view();
            return ::mpz_cmp(&n1.m_int.g_dy(), v2) < 0;
        } else {
            return ::mpz_cmp(&n1.m_int.g_dy(), &n2.m_int.g_dy()) < 0;
        }
    }
    template <typename Integer>
    static bool binary_less_than(const mp_integer &n1, const Integer &n2,
                                 typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        return n1 < mp_integer(n2);
    }
    template <typename Integer>
    static bool binary_less_than(const Integer &n1, const mp_integer &n2,
                                 typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        return mp_integer(n1) < n2;
    }
    template <typename FloatingPoint>
    static bool
    binary_less_than(const mp_integer &n, const FloatingPoint &x,
                     typename std::enable_if<std::is_floating_point<FloatingPoint>::value>::type * = nullptr)
    {
        return static_cast<FloatingPoint>(n) < x;
    }
    template <typename FloatingPoint>
    static bool
    binary_less_than(const FloatingPoint &x, const mp_integer &n,
                     typename std::enable_if<std::is_floating_point<FloatingPoint>::value>::type * = nullptr)
    {
        return x < static_cast<FloatingPoint>(n);
    }
    static bool binary_leq(const mp_integer &n1, const mp_integer &n2)
    {
        const bool s1 = n1.is_static(), s2 = n2.is_static();
        if (s1 && s2) {
            return (n1.m_int.g_st() <= n2.m_int.g_st());
        } else if (s1 && !s2) {
            auto v1 = n1.m_int.g_st().get_mpz_view();
            return ::mpz_cmp(v1, &n2.m_int.g_dy()) <= 0;
        } else if (!s1 && s2) {
            auto v2 = n2.m_int.g_st().get_mpz_view();
            return ::mpz_cmp(&n1.m_int.g_dy(), v2) <= 0;
        } else {
            return ::mpz_cmp(&n1.m_int.g_dy(), &n2.m_int.g_dy()) <= 0;
        }
    }
    template <typename Integer>
    static bool binary_leq(const mp_integer &n1, const Integer &n2,
                           typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        return n1 <= mp_integer(n2);
    }
    template <typename Integer>
    static bool binary_leq(const Integer &n1, const mp_integer &n2,
                           typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr)
    {
        return mp_integer(n1) <= n2;
    }
    template <typename FloatingPoint>
    static bool binary_leq(const mp_integer &n, const FloatingPoint &x,
                           typename std::enable_if<std::is_floating_point<FloatingPoint>::value>::type * = nullptr)
    {
        return static_cast<FloatingPoint>(n) <= x;
    }
    template <typename FloatingPoint>
    static bool binary_leq(const FloatingPoint &x, const mp_integer &n,
                           typename std::enable_if<std::is_floating_point<FloatingPoint>::value>::type * = nullptr)
    {
        return x <= static_cast<FloatingPoint>(n);
    }
    // Exponentiation.
    // ebs = exponentiation by squaring.
    mp_integer ebs(unsigned long n) const
    {
        mp_integer result(1), tmp(*this);
        while (n) {
            if (n % 2ul) {
                result *= tmp;
            }
            tmp *= tmp;
            n /= 2ul;
        }
        return result;
    }
    template <typename T>
    mp_integer
    pow_impl(const T &ui,
             typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type * = nullptr) const
    {
        unsigned long exp;
        try {
            exp = boost::numeric_cast<unsigned long>(ui);
        } catch (const boost::numeric::bad_numeric_cast &) {
            piranha_throw(std::invalid_argument, "invalid argument for exponentiation");
        }
        return ebs(exp);
    }
    template <typename T>
    mp_integer
    pow_impl(const T &si,
             typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type * = nullptr) const
    {
        return pow_impl(mp_integer(si));
    }
    mp_integer pow_impl(const mp_integer &n) const
    {
        if (n.sign() >= 0) {
            unsigned long exp;
            try {
                exp = static_cast<unsigned long>(n);
            } catch (const std::overflow_error &) {
                piranha_throw(std::invalid_argument, "invalid argument for exponentiation");
            }
            return pow_impl(exp);
        } else {
            return 1 / pow_impl(-n);
        }
    }
    // mpz view class.
    class mpz_view
    {
        using static_mpz_view = typename detail::integer_union<NBits>::s_storage::template static_mpz_view<>;

    public:
        explicit mpz_view(const mp_integer &n)
            : m_static_view(n.is_static() ? n.m_int.g_st().get_mpz_view() : static_mpz_view{}),
              m_dyn_ptr(n.is_static() ? nullptr : &(n.m_int.g_dy()))
        {
        }
        mpz_view(const mpz_view &) = delete;
        mpz_view(mpz_view &&) = default;
        mpz_view &operator=(const mpz_view &) = delete;
        mpz_view &operator=(mpz_view &&) = delete;
        operator const detail::mpz_struct_t *() const
        {
            return get();
        }
        const detail::mpz_struct_t *get() const
        {
            if (m_dyn_ptr) {
                return m_dyn_ptr;
            } else {
                return m_static_view;
            }
        }

    private:
        static_mpz_view m_static_view;
        const detail::mpz_struct_t *m_dyn_ptr;
    };
    // Serialization support.
    friend class boost::serialization::access;
    template <class Archive>
    void save(Archive &ar, unsigned int) const
    {
        std::ostringstream oss;
        oss << *this;
        auto s = oss.str();
        ar &s;
    }
    template <class Archive>
    void load(Archive &ar, unsigned int)
    {
        std::string s;
        ar &s;
        *this = mp_integer(s);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
    /// Defaulted default constructor.
    /**
     * The value of the integer will be initialised to 0.
     */
    mp_integer() = default;
    /// Defaulted copy constructor.
    mp_integer(const mp_integer &) = default;
    /// Defaulted move constructor.
    mp_integer(mp_integer &&) = default;
    /// Generic constructor.
    /**
     * \note
     * This constructor is enabled only if \p T is an interoperable type.
     *
     * Construction from a floating-point type will result in the truncated
     * counterpart of the original value.
     *
     * @param[in] x object used to construct \p this.
     *
     * @throws std::invalid_argument if the construction fails (e.g., construction from a non-finite
     * floating-point value).
     */
    template <typename T, generic_ctor_enabler<T> = 0>
    explicit mp_integer(const T &x)
    {
        construct_from_interoperable(x);
    }
    /// Constructor from C string.
    /**
     * The string must be a sequence of decimal digits, preceded by a minus sign for
     * strictly negative numbers. The first digit of a non-zero number must not be zero. A malformed string will throw
     * an \p std::invalid_argument
     * exception.
     *
     * Note that if the string is not null-terminated, undefined behaviour will occur.
     *
     * @param[in] str decimal string representation of the number used to initialise the integer object.
     *
     * @throws std::invalid_argument if the string is malformed.
     */
    explicit mp_integer(const char *str)
    {
        construct_from_string(str);
    }
    /// Constructor from C++ string.
    /**
     * Equivalent to the constructor from C string.
     *
     * @param[in] str decimal string representation of the number used to initialise the integer object.
     *
     * @throws unspecified any exception thrown by the constructor from C string.
     */
    explicit mp_integer(const std::string &str)
    {
        construct_from_string(str.c_str());
    }
    /// Defaulted destructor.
    ~mp_integer() = default;
    /// Defaulted copy-assignment operator.
    mp_integer &operator=(const mp_integer &) = default;
    /// Defaulted move-assignment operator.
    mp_integer &operator=(mp_integer &&) = default;
    /// Generic assignment operator.
    /**
     * \note
     * This operator is enabled only if \p T is an interoperable type.
     *
     * This assignment operator is equivalent to constructing a temporary instance of mp_integer from \p x
     * and then move-assigning it to \p this.
     *
     * @param[in] x object that will be assigned to \p this.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor of mp_integer.
     */
    template <typename T, generic_ctor_enabler<T> = 0>
    mp_integer &operator=(const T &x)
    {
        return (*this = mp_integer(x));
    }
    /// Assignment from C++ string.
    /**
     * Equivalent to the construction and susbequent move to \p this of a temporary mp_integer from \p str.
     *
     * @param[in] str C++ string that will be assigned to \p this.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the constructor of mp_integer from string.
     */
    mp_integer &operator=(const std::string &str)
    {
        operator=(mp_integer(str));
        return *this;
    }
    /// Assignment from C string.
    /**
     * Equivalent to the construction and susbequent move to \p this of a temporary mp_integer from \p str.
     *
     * @param[in] str C string that will be assigned to \p this.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the constructor of mp_integer from string.
     */
    mp_integer &operator=(const char *str)
    {
        operator=(mp_integer(str));
        return *this;
    }
    /// Get an \p mpz view of \p this.
    /**
     * This method will return an object of an unspecified type \p mpz_view which is implicitly convertible
     * to a const pointer to an \p mpz struct (and which can thus be used as a <tt>const mpz_t</tt>
     * parameter in GMP functions). In addition to the implicit conversion operator, the \p mpz struct pointer
     * can also be retrieved via the <tt>get()</tt> method of the \p mpz_view class.
     * The pointee will represent a GMP integer whose value is equal to \p this.
     *
     * It is important to keep in mind the following facts about the returned \p mpz_view object:
     * - \p mpz_view objects can only be move-constructed (the other constructors and the assignment operators
     *   are disabled);
     * - the returned object and the pointer returned by its <tt>get()</tt> method might reference internal data
     *   belonging to \p this, and they can thus be used safely only during the lifetime of \p this;
     * - the lifetime of the pointer returned by the <tt>get()</tt> method is tied to the lifetime of the \p mpz_view
     *   object (that is, if the \p mpz_view object is destroyed, any pointer previously returned by <tt>get()</tt>
     *   becomes invalid);
     * - any modification to \p this will also invalidate the view and the pointer.
     *
     * @return an \p mpz view of \p this.
     */
    mpz_view get_mpz_view() const
    {
        return mpz_view(*this);
    }
    /// Conversion operator.
    /**
     * \note
     * This operator is enabled only if \p T is an interoperable type.
     *
     * Conversion to integral types, if possible, will always be exact. Conversion to \p bool produces
     * \p true for nonzero values, \p false for zero. Conversion to floating-point types is performed
     * via arithmetic operations and might generate infinities in case the value is too large.
     *
     * @return the value of \p this converted to type \p T.
     *
     * @throws std::overflow_error if the conversion fails (e.g., the range of the target integral type
     * is insufficient to represent the value of <tt>this</tt>).
     * @throws unspecified any exception raised by <tt>boost::numeric_cast()</tt> in case of (unlikely)
     * overflow errors while converting between integral types.
     */
    template <typename T, cast_enabler<T> = 0>
    explicit operator T() const
    {
        return convert_to_impl<T>();
    }
    /// Overload output stream operator for piranha::mp_integer.
    /**
     * The input \p n will be directed to the output stream \p os as a string of digits in base 10.
     *
     * @param[in] os output stream.
     * @param[in] n piranha::mp_integer to be directed to stream.
     *
     * @return reference to \p os.
     *
     * @throws std::invalid_argument if the number of digits is larger than an implementation-defined maximum.
     * @throws unspecified any exception thrown by <tt>std::vector::resize()</tt>.
     */
    friend std::ostream &operator<<(std::ostream &os, const mp_integer &n)
    {
        if (n.m_int.is_static()) {
            return (os << n.m_int.g_st());
        } else {
            return detail::stream_mpz(os, n.m_int.g_dy());
        }
    }
    /// Overload input stream operator for piranha::mp_integer.
    /**
     * Equivalent to extracting a line from the stream and then assigning it to \p n.
     *
     * @param[in] is input stream.
     * @param[in,out] n integer to which the contents of the stream will be assigned.
     *
     * @return reference to \p is.
     *
     * @throws unspecified any exception thrown by the constructor from string of piranha::mp_integer.
     */
    friend std::istream &operator>>(std::istream &is, mp_integer &n)
    {
        std::string tmp_str;
        std::getline(is, tmp_str);
        n = tmp_str;
        return is;
    }
    /// Promote to dynamic storage.
    /**
     * This method will promote \p this to dynamic storage, if \p this is currently stored in static
     * storage. Otherwise, an error will be raised.
     *
     * @throws std::invalid_argument if \p this is not currently stored in static storage.
     */
    void promote()
    {
        if (unlikely(!m_int.is_static())) {
            piranha_throw(std::invalid_argument, "cannot promote non-static integer");
        }
        m_int.promote();
    }
    /// Test storage status.
    /**
     * @return \p true if \p this is currently stored in static storage, \p false otherwise.
     */
    bool is_static() const
    {
        return m_int.is_static();
    }
    /// Negate in-place.
    void negate()
    {
        if (is_static()) {
            m_int.g_st().negate();
        } else {
            ::mpz_neg(&m_int.g_dy(), &m_int.g_dy());
        }
    }
    /// Sign.
    /**
     * @return 1 if <tt>this > 0</tt>, 0 if <tt>this == 0</tt> and -1 if <tt>this < 0</tt>.
     */
    int sign() const
    {
        if (is_static()) {
            if (m_int.g_st()._mp_size > 0) {
                return 1;
            }
            if (m_int.g_st()._mp_size < 0) {
                return -1;
            }
            return 0;
        } else {
            return mpz_sgn(&m_int.g_dy());
        }
    }
    /// In-place addition.
    /**
     * \note
     * This operator is enabled only if \p T is an interoperable type or piranha::mp_integer.
     *
     * Add \p x in-place. If \p T is piranha::mp_integer or an integral type, the result will be exact. If \p T is a
     * floating-point type, the following
     * sequence of operations takes place:
     *
     * - \p this is converted to an instance \p f of type \p T via the conversion operator,
     * - \p f is added to \p x,
     * - the result is assigned back to \p this.
     *
     * @param[in] x argument for the addition.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, the conversion operator or the generic
     * assignment operator, if used.
     */
    template <typename T>
    auto operator+=(const T &x) -> decltype(this->in_place_add(x))
    {
        return in_place_add(x);
    }
    /// Generic in-place addition with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const interoperable type.
     *
     * Add a piranha::mp_integer in-place. This method will first compute <tt>n + x</tt>, cast it back to \p T via \p
     * static_cast and finally assign the result to \p x.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_enabler<T> = 0>
    friend T &operator+=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(n + x);
        return x;
    }
    /// Generic binary addition involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If no floating-point types are involved, the exact result of the operation will be returned as a
     * piranha::mp_integer.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and added to \p f to generate the return value, which will then be of type \p F.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x + y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor or the conversion operator, if used.
     */
    template <typename T, typename U>
    friend auto operator+(const T &x, const U &y) -> decltype(mp_integer::binary_plus(x, y))
    {
        return binary_plus(x, y);
    }
    /// Identity operation.
    /**
     * @return copy of \p this.
     */
    mp_integer operator+() const
    {
        return *this;
    }
    /// Prefix increment.
    /**
     * Increment \p this by one.
     *
     * @return reference to \p this after the increment.
     */
    mp_integer &operator++()
    {
        return operator+=(1);
    }
    /// Suffix increment.
    /**
     * Increment \p this by one and return a copy of \p this as it was before the increment.
     *
     * @return copy of \p this before the increment.
     */
    mp_integer operator++(int)
    {
        const mp_integer retval(*this);
        ++(*this);
        return retval;
    }
    /// Addition in ternary form.
    /**
     * Sets \p this to <tt>n1 + n2</tt>. This form can be more efficient than the corresponding binary operator.
     *
     * @param[in] n1 first argument.
     * @param[in] n2 second argument.
     *
     * @return reference to \p this.
     */
    mp_integer &add(const mp_integer &n1, const mp_integer &n2)
    {
        bool s0 = is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s0 && s1 && s2) {
            if (likely(!detail::integer_union<NBits>::s_storage::add(m_int.g_st(), n1.m_int.g_st(), n2.m_int.g_st()))) {
                return *this;
            }
        }
        // this will have to be mpz in any case, promote it if needed and re-check the
        // static flags in case this coincides with n1 and/or n2.
        if (s0) {
            m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // 2**2 possibilities.
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        switch (mask) {
            case 0u: {
                auto v1 = n1.m_int.g_st().get_mpz_view(), v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_add(&m_int.g_dy(), v1, v2);
                break;
            }
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_add(&m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_add(&m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_add(&m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
        return *this;
    }
    /// In-place subtraction.
    /**
     * \note
     * This operator is enabled only if \p T is an interoperable type or piranha::mp_integer.
     *
     * Subtract \p x in-place. If \p T is piranha::mp_integer or an integral type, the result will be exact. If \p T is
     * a floating-point type, the following
     * sequence of operations takes place:
     *
     * - \p this is converted to an instance \p f of type \p T via the conversion operator,
     * - \p x is subtracted from \p f,
     * - the result is assigned back to \p this.
     *
     * @param[in] x argument for the subtraction.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, the conversion operator or the generic
     * assignment operator, if used.
     */
    template <typename T>
    auto operator-=(const T &x) -> decltype(this->in_place_sub(x))
    {
        return in_place_sub(x);
    }
    /// Generic in-place subtraction with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const interoperable type.
     *
     * Subtract a piranha::mp_integer in-place. This method will first compute <tt>x - n</tt>, cast it back to \p T via
     * \p static_cast and finally assign the result to \p x.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_enabler<T> = 0>
    friend T &operator-=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(x - n);
        return x;
    }
    /// Generic binary subtraction involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and subtracted from (or to) \p f to generate the return value, which will then be of type \p F.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x - y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor or the conversion operator, if used.
     */
    template <typename T, typename U>
    friend auto operator-(const T &x, const U &y) -> decltype(mp_integer::binary_subtract(x, y))
    {
        return binary_subtract(x, y);
    }
    /// Negated copy.
    /**
     * @return copy of \p -this.
     */
    mp_integer operator-() const
    {
        mp_integer retval(*this);
        retval.negate();
        return retval;
    }
    /// Prefix decrement.
    /**
     * Decrement \p this by one and return.
     *
     * @return reference to \p this.
     */
    mp_integer &operator--()
    {
        return operator-=(1);
    }
    /// Suffix decrement.
    /**
     * Decrement \p this by one and return a copy of \p this as it was before the decrement.
     *
     * @return copy of \p this before the decrement.
     */
    mp_integer operator--(int)
    {
        const mp_integer retval(*this);
        --(*this);
        return retval;
    }
    /// Subtraction in ternary form.
    /**
     * Sets \p this to <tt>n1 - n2</tt>. This form can be more efficient than the corresponding binary operator.
     *
     * @param[in] n1 first argument.
     * @param[in] n2 second argument.
     *
     * @return reference to \p this.
     */
    mp_integer &sub(const mp_integer &n1, const mp_integer &n2)
    {
        bool s0 = is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s0 && s1 && s2) {
            if (likely(!detail::integer_union<NBits>::s_storage::sub(m_int.g_st(), n1.m_int.g_st(), n2.m_int.g_st()))) {
                return *this;
            }
        }
        // this will have to be mpz in any case, promote it if needed and re-check the
        // static flags in case this coincides with n1 and/or n2.
        if (s0) {
            m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // 2**2 possibilities.
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        switch (mask) {
            case 0u: {
                auto v1 = n1.m_int.g_st().get_mpz_view(), v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_sub(&m_int.g_dy(), v1, v2);
                break;
            }
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_sub(&m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_sub(&m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_sub(&m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
        return *this;
    }
    /// In-place multiplication.
    /**
     * \note
     * This operator is enabled only if \p T is an interoperable type or piranha::mp_integer.
     *
     * Multiply by \p x in-place. If \p T is piranha::mp_integer or an integral type, the result will be exact. If \p T
     * is a floating-point type, the following
     * sequence of operations takes place:
     *
     * - \p this is converted to an instance \p f of type \p T via the conversion operator,
     * - \p x is multiplied by \p f,
     * - the result is assigned back to \p this.
     *
     * @param[in] x argument for the multiplication.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, the conversion operator or the generic
     * assignment operator, if used.
     */
    template <typename T>
    auto operator*=(const T &x) -> decltype(this->in_place_mul(x))
    {
        return in_place_mul(x);
    }
    /// Generic in-place multiplication with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const interoperable type.
     *
     * Multiply by a piranha::mp_integer in-place. This method will first compute <tt>x * n</tt>, cast it back to \p T
     * via \p static_cast and finally assign the result to \p x.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_enabler<T> = 0>
    friend T &operator*=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(x * n);
        return x;
    }
    /// Generic binary multiplication involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and multiplied by \p f to generate the return value, which will then be of type \p F.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x * y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor or the conversion operator, if used.
     */
    template <typename T, typename U>
    friend auto operator*(const T &x, const U &y) -> decltype(mp_integer::binary_mul(x, y))
    {
        return binary_mul(x, y);
    }
    /// Combined multiply-add.
    /**
     * Sets \p this to <tt>this + (n1 * n2)</tt>.
     *
     * @param[in] n1 first argument.
     * @param[in] n2 second argument.
     *
     * @return reference to \p this.
     */
    mp_integer &multiply_accumulate(const mp_integer &n1, const mp_integer &n2)
    {
        bool s0 = is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s0 && s1 && s2) {
            if (likely(!m_int.g_st().multiply_accumulate(n1.m_int.g_st(), n2.m_int.g_st()))) {
                return *this;
            }
        }
        // this will have to be mpz in any case, promote it if needed and re-check the
        // static flags in case this coincides with n1 and/or n2.
        if (s0) {
            m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // 2**2 possibilities.
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        switch (mask) {
            case 0u: {
                auto v1 = n1.m_int.g_st().get_mpz_view(), v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_addmul(&m_int.g_dy(), v1, v2);
                break;
            }
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_addmul(&m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_addmul(&m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_addmul(&m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
        return *this;
    }
    /// Multiplication in ternary form.
    /**
     * Sets \p this to <tt>n1 * n2</tt>. This form can be more efficient than the corresponding binary operator.
     *
     * @param[in] n1 first argument.
     * @param[in] n2 second argument.
     *
     * @return reference to \p this.
     */
    mp_integer &mul(const mp_integer &n1, const mp_integer &n2)
    {
        bool s0 = is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s0 && s1 && s2) {
            if (likely(!detail::integer_union<NBits>::s_storage::mul(m_int.g_st(), n1.m_int.g_st(), n2.m_int.g_st()))) {
                return *this;
            }
        }
        // this will have to be mpz in any case, promote it if needed and re-check the
        // static flags in case this coincides with n1 and/or n2.
        if (s0) {
            m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // 2**2 possibilities.
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        switch (mask) {
            case 0u: {
                auto v1 = n1.m_int.g_st().get_mpz_view(), v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_mul(&m_int.g_dy(), v1, v2);
                break;
            }
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_mul(&m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_mul(&m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_mul(&m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
        return *this;
    }
    /// In-place division.
    /**
     * \note
     * This operator is enabled only if \p T is an interoperable type or piranha::mp_integer.
     *
     * Divide by \p x in-place. If \p T is piranha::mp_integer or an integral type, the result will be truncated
     * (i.e., rounded towards 0). If \p T is a floating-point type, the following
     * sequence of operations takes place:
     *
     * - \p this is converted to an instance \p f of type \p T via the conversion operator,
     * - \p f is divided by \p x,
     * - the result is assigned back to \p this.
     *
     * @param[in] x argument for the division.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, the conversion operator or the generic
     * assignment operator, if used.
     * @throws piranha::zero_division_error if \p T is an integral type and \p x is zero (as established by
     * piranha::math::is_zero()).
     */
    template <typename T>
    auto operator/=(const T &x) -> decltype(this->in_place_div(x))
    {
        if (unlikely(math::is_zero(x))) {
            piranha_throw(zero_division_error, "division by zero in mp_integer");
        }
        return in_place_div(x);
    }
    /// Generic in-place division with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const interoperable type.
     *
     * Divide by a piranha::mp_integer in-place. This method will first compute <tt>x / n</tt>, cast it back to \p T via
     * \p static_cast and finally assign the result to \p x.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_enabler<T> = 0>
    friend T &operator/=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(x / n);
        return x;
    }
    /// Generic binary division involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and divided by (or used as a dividend for) \p f to generate the return value, which will then be of type \p F.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x / y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor or the conversion operator, if used.
     * @throws piranha::zero_division_error if both operands are of integral type and a division by zero occurs.
     */
    template <typename T, typename U>
    friend auto operator/(const T &x, const U &y) -> decltype(mp_integer::binary_div(x, y))
    {
        if (unlikely(math::is_zero(y))) {
            piranha_throw(zero_division_error, "division by zero in mp_integer");
        }
        return binary_div(x, y);
    }
    /// Division in ternary form.
    /**
     * Sets \p this to <tt>n1 / n2</tt>. This form can be more efficient than the corresponding binary operator.
     *
     * @param[in] n1 first argument.
     * @param[in] n2 second argument.
     *
     * @throws piranha::zero_division_error if \p n2 is zero.
     *
     * @return reference to \p this.
     */
    mp_integer &div(const mp_integer &n1, const mp_integer &n2)
    {
        if (unlikely(math::is_zero(n2))) {
            piranha_throw(zero_division_error, "division by zero");
        }
        bool s0 = is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s1 && s2) {
            if (!s0) {
                // NOTE: this is safe, as this is not static while n1 and n2 are,
                // and thus this cannot overlap with n1 or n2.
                *this = mp_integer{};
            }
            mp_integer r;
            // NOTE: the static division routine here does both division and remainder, we
            // need to check if there's performance to be gained by doing only the division.
            m_int.g_st().div(m_int.g_st(), r.m_int.g_st(), n1.m_int.g_st(), n2.m_int.g_st());
            return *this;
        }
        // this will have to be mpz in any case, promote it if needed and re-check the
        // static flags in case this coincides with n1 and/or n2.
        if (s0) {
            m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        piranha_assert(mask > 0u);
        switch (mask) {
            // NOTE: case 0 here is not possible as it would mean that n1 and n2 are both static,
            // but we handled the case above.
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_tdiv_q(&m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_tdiv_q(&m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_tdiv_q(&m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
        return *this;
    }
    /// Division with remainder.
    /**
     * This method will set \p q to the quotient and \p r to the remainder of <tt>n1 / n2</tt>.
     * The sign of the remainder will be the sign of the numerator \p n1.
     *
     * @param[out] q the quotient.
     * @param[out] r the remainder.
     * @param[in] n1 the numerator.
     * @param[in] n2 the denominator.
     *
     * @throws piranha::zero_division_error if \p n2 is zero.
     * @throws std::invalid_argument if \p q and \p r are the same object.
     */
    static void divrem(mp_integer &q, mp_integer &r, const mp_integer &n1, const mp_integer &n2)
    {
        if (unlikely(math::is_zero(n2))) {
            piranha_throw(zero_division_error, "division by zero");
        }
        if (unlikely(&q == &r)) {
            piranha_throw(std::invalid_argument, "quotient and remainder cannot be the same object");
        }
        // First we make sure that both q and r have the same storage.
        bool s0 = q.is_static();
        if (s0 != r.is_static()) {
            if (s0) {
                // q is static, r is dynamic. Promote q.
                q.promote();
                s0 = false;
            } else {
                // q is dynamic, r static. Promote r.
                r.promote();
            }
        }
        piranha_assert(s0 == q.is_static() || q.is_static() == r.is_static());
        bool s1 = n1.is_static(), s2 = n2.is_static();
        // If n1 and n2 are both statics, we can carry the computation completely in static storage.
        if (s1 && s2) {
            if (!s0) {
                q = mp_integer{};
                r = mp_integer{};
            }
            q.m_int.g_st().div(q.m_int.g_st(), r.m_int.g_st(), n1.m_int.g_st(), n2.m_int.g_st());
            return;
        }
        // If either n1 or n2 are static, we need to promote the return values and re-check n1/n2.
        if (s0) {
            q.promote();
            r.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        piranha_assert(mask > 0u);
        switch (mask) {
            // NOTE: case 0 here is not possible as it would mean that n1 and n2 are both static,
            // but we handled the case above.
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_tdiv_qr(&q.m_int.g_dy(), &r.m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_tdiv_qr(&q.m_int.g_dy(), &r.m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_tdiv_qr(&q.m_int.g_dy(), &r.m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
    }
    /// In-place modulo operation.
    /**
     * \note
     * This template operator is enabled only if \p T is piranha::mp_integer or an integral type among the interoperable
     * types.
     *
     * Sets \p this to <tt>this % n</tt>. This operator behaves in the way specified by the C++ standard (specifically,
     * the sign of the remainder will be the sign of the numerator).
     *
     * @param[in] n argument for the modulo operation.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, if used.
     * @throws piranha::zero_division_error if <tt>n == 0</tt>.
     */
    template <typename T>
    auto operator%=(const T &n) -> decltype(this->in_place_mod(n))
    {
        if (unlikely(math::is_zero(n))) {
            piranha_throw(zero_division_error, "division by zero in mp_integer");
        }
        return in_place_mod(n);
    }
    /// Generic in-place modulo with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const integral interoperable type.
     *
     * Compute the remainder with respect to a piranha::mp_integer in-place. This method will first compute <tt>x %
     * n</tt>,
     * cast it back to \p T via \p static_cast and finally assign the result to \p x.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_mod_enabler<T> = 0>
    friend T &operator%=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(x % n);
        return x;
    }
    /// Generic binary modulo operation involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an integral interoperable type,
     * - \p U is piranha::mp_integer and \p T is an integral interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x % y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor, if used.
     * @throws piranha::zero_division_error if the second operand is zero.
     */
    template <typename T, typename U>
    friend auto operator%(const T &x, const U &y) -> decltype(mp_integer::binary_mod(x, y))
    {
        if (unlikely(math::is_zero(y))) {
            piranha_throw(zero_division_error, "division by zero in mp_integer");
        }
        return binary_mod(x, y);
    }
    /// In-place left shift operation.
    /**
     * \note
     * This template operator is enabled only if \p T is piranha::mp_integer or an integral type.
     *
     * Sets \p this to <tt>this << n</tt>. The left shift operation is equivalent to a multiplication
     * by 2 to the power of \p n.
     *
     * @param[in] n argument for the left shift operation.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, if used.
     * @throws piranha::invalid_argument if <tt>n</tt> is negative or it does not fit in an <tt>mp_bitcnt_t</tt>.
     */
    template <typename T>
    auto operator<<=(const T &n) -> decltype(this->in_place_lshift(n))
    {
        return in_place_lshift(n);
    }
    /// Generic in-place left shift with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const integral interoperable type.
     *
     * Compute the left shift with respect to a piranha::mp_integer in-place. This method will first compute <tt>x <<
     * n</tt>,
     * cast it back to \p T via \p static_cast and finally assign the result to \p x. The left shift operation is
     * equivalent to a multiplication
     * by 2 to the power of \p n.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_shift_enabler<T> = 0>
    friend T &operator<<=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(x << n);
        return x;
    }
    /// Generic binary left shift operation involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an integral interoperable type,
     * - \p U is piranha::mp_integer and \p T is an integral interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x << y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor, if used.
     */
    template <typename T, typename U>
    friend auto operator<<(const T &x, const U &y) -> decltype(mp_integer::binary_lshift(x, y))
    {
        return binary_lshift(x, y);
    }
    /// In-place right shift operation.
    /**
     * \note
     * This template operator is enabled only if \p T is piranha::mp_integer or an integral type.
     *
     * Sets \p this to <tt>this >> n</tt>. The right shift operation is equivalent to a truncated division
     * by 2 to the power of \p n.
     *
     * @param[in] n argument for the right shift operation.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the generic constructor, if used.
     * @throws piranha::invalid_argument if <tt>n</tt> is negative or it does not fit in an <tt>mp_bitcnt_t</tt>.
     */
    template <typename T>
    auto operator>>=(const T &n) -> decltype(this->in_place_rshift(n))
    {
        return in_place_rshift(n);
    }
    /// Generic in-place right shift with piranha::mp_integer.
    /**
     * \note
     * This operator is enabled only if \p T is a non-const integral interoperable type.
     *
     * Compute the right shift with respect to a piranha::mp_integer in-place. This method will first compute <tt>x >>
     * n</tt>,
     * cast it back to \p T via \p static_cast and finally assign the result to \p x. The right shift operation is
     * equivalent to a
     * truncated division by 2 to the power of \p n.
     *
     * @param[in,out] x first argument.
     * @param[in] n second argument.
     *
     * @return reference to \p x.
     *
     * @throws unspecified any exception thrown by the binary operator or by casting piranha::mp_integer to \p T.
     */
    template <typename T, generic_in_place_shift_enabler<T> = 0>
    friend T &operator>>=(T &x, const mp_integer &n)
    {
        x = static_cast<T>(x >> n);
        return x;
    }
    /// Generic binary right shift operation involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an integral interoperable type,
     * - \p U is piranha::mp_integer and \p T is an integral interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return <tt>x >> y</tt>.
     *
     * @throws unspecified any exception thrown by:
     * - the corresponding in-place operator,
     * - the invoked constructor, if used.
     */
    template <typename T, typename U>
    friend auto operator>>(const T &x, const U &y) -> decltype(mp_integer::binary_rshift(x, y))
    {
        return binary_rshift(x, y);
    }
    /// Generic equality operator involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If no floating-point types are involved, the exact result of the comparison will be returned.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and compared to \p f to generate the return value.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return \p true if <tt>x == y</tt>, \p false otherwise.
     *
     * @throws unspecified any exception thrown by the invoked constructor or the conversion operator, if used.
     */
    template <typename T, typename U>
    friend auto operator==(const T &x, const U &y) -> decltype(mp_integer::binary_equality(x, y))
    {
        return binary_equality(x, y);
    }
    /// Generic inequality operator involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * This operator is the negation of operator==().
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return \p true if <tt>x == y</tt>, \p false otherwise.
     *
     * @throws unspecified any exception thrown by operator==().
     */
    template <typename T, typename U>
    friend auto operator!=(const T &x, const U &y) -> decltype(!mp_integer::binary_equality(x, y))
    {
        return !binary_equality(x, y);
    }
    /// Generic less-than operator involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If no floating-point types are involved, the exact result of the comparison will be returned.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and compared to \p f to generate the return value.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return \p true if <tt>x < y</tt>, \p false otherwise.
     *
     * @throws unspecified any exception thrown by the invoked constructor or the conversion operator, if used.
     */
    template <typename T, typename U>
    friend auto operator<(const T &x, const U &y) -> decltype(mp_integer::binary_less_than(x, y))
    {
        return binary_less_than(x, y);
    }
    /// Generic less-than or equal operator involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If no floating-point types are involved, the exact result of the comparison will be returned.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and compared to \p f to generate the return value.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return \p true if <tt>x <= y</tt>, \p false otherwise.
     *
     * @throws unspecified any exception thrown by the invoked constructor or the conversion operator, if used.
     */
    template <typename T, typename U>
    friend auto operator<=(const T &x, const U &y) -> decltype(mp_integer::binary_leq(x, y))
    {
        return binary_leq(x, y);
    }
    /// Generic greater-than operator involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If no floating-point types are involved, the exact result of the comparison will be returned.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and compared to \p f to generate the return value.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return \p true if <tt>x > y</tt>, \p false otherwise.
     *
     * @throws unspecified any exception thrown by operator<().
     */
    template <typename T, typename U>
    friend auto operator>(const T &x, const U &y) -> decltype(mp_integer::binary_less_than(y, x))
    {
        return binary_less_than(y, x);
    }
    /// Generic greater-than or equal operator involving piranha::mp_integer.
    /**
     * \note
     * This template operator is enabled only if either:
     * - \p T is piranha::mp_integer and \p U is an interoperable type,
     * - \p U is piranha::mp_integer and \p T is an interoperable type,
     * - both \p T and \p U are piranha::mp_integer.
     *
     * If no floating-point types are involved, the exact result of the comparison will be returned.
     *
     * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an
     * instance of type \p F
     * and compared to \p f to generate the return value.
     *
     * @param[in] x first argument
     * @param[in] y second argument.
     *
     * @return \p true if <tt>x >= y</tt>, \p false otherwise.
     *
     * @throws unspecified any exception thrown by operator<=().
     */
    template <typename T, typename U>
    friend auto operator>=(const T &x, const U &y) -> decltype(mp_integer::binary_leq(y, x))
    {
        return binary_leq(y, x);
    }
    /// Exponentiation.
    /**
     * \note
     * This template method is activated only if \p T is an integral type or piranha::mp_integer.
     *
     * Return <tt>this ** exp</tt>.  Negative
     * powers are calculated as <tt>(1 / this) ** exp</tt>. Trying to raise zero to a negative exponent will throw a
     * piranha::zero_division_error exception. <tt>this ** 0</tt> will always return 1.
     *
     * The value of \p exp cannot exceed in absolute value the maximum value representable by the <tt>unsigned long</tt>
     * type, otherwise an
     * \p std::invalid_argument exception will be thrown.
     *
     * @param[in] exp exponent.
     *
     * @return <tt>this ** exp</tt>.
     *
     * @throws std::invalid_argument if <tt>exp</tt>'s value is outside the range of the <tt>unsigned long</tt> type.
     * @throws piranha::zero_division_error if \p this is zero and \p exp is negative.
     * @throws unspecified any exception thrown by the generic constructor, if used.
     */
    template <typename T>
    auto pow(const T &exp) const -> decltype(this->pow_impl(exp))
    {
        return pow_impl(exp);
    }
    /// Absolute value.
    /**
     * @return absolute value of \p this.
     */
    mp_integer abs() const
    {
        mp_integer retval(*this);
        if (retval.is_static()) {
            if (retval.m_int.g_st()._mp_size < 0) {
                retval.m_int.g_st()._mp_size = -retval.m_int.g_st()._mp_size;
            }
        } else {
            ::mpz_abs(&retval.m_int.g_dy(), &retval.m_int.g_dy());
        }
        return retval;
    }
    /// GCD.
    /**
     * This method will write to \p out the GCD of \p n1 and \p n2.
     * The returned value is guaranteed to be non-negative if both arguments are non-negative.
     *
     * @param[out] out the output value.
     * @param[in] n1 first argument
     * @param[in] n2 second argument.
     */
    static void gcd(mp_integer &out, const mp_integer &n1, const mp_integer &n2)
    {
        // NOTE: this function would be a good candidate for demotion.
        bool s0 = out.is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s1 && s2) {
            // Go with the euclidean computation if both are statics. This will result
            // in a static out as well.
            out = detail::gcd_euclidean(n1, n2);
            return;
        }
        // We will set out to mpz in any case, promote it if needed and re-check the
        // static flags in case this coincides with n1 and/or n2.
        if (s0) {
            out.m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        piranha_assert(mask > 0u);
        switch (mask) {
            // NOTE: case 0 here is not possible as it would mean that n1 and n2 are both static,
            // but we handled the case above.
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_gcd(&out.m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_gcd(&out.m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_gcd(&out.m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
    }

private:
    struct hash_checks {
        static const unsigned nbits_size_t = static_cast<unsigned>(std::numeric_limits<std::size_t>::digits);
        using s_storage = typename detail::integer_union<NBits>::s_storage;
        // Check that the computation of the total number of bits does not overflow when the number
        // of size_t to extract is no more than the corresponding quantity for the static int.
        // This protects again both the computation of tot_nbits, but also the multiplication inside
        // read_uint as return type coincides with std::size_t.
        static const bool value
            = s_storage::hash_checks::max_n_size_t < std::numeric_limits<std::size_t>::max() / nbits_size_t;
    };

public:
    /// Hash value
    /**
     * The returned hash value does not depend on how the integer is stored (i.e., dynamic or static
     * storage).
     *
     * @return a hash value for \p this.
     */
    std::size_t hash() const
    {
        // NOTE: in order to check that this method can be called safely we need to make sure that:
        // - the static hash calculation has no overflows (checked above),
        // - dynamic hash calculation cannot overflow when operating on integers with size (in terms
        //   of std::size_t elements) in the static range.
        static_assert(hash_checks::value, "Overflow error.");
        // NOTE: a possible performance improvement here is to go and read directly
        // the first size_t from the storage if the value is positive. No loops and
        // no hash combining.
        if (is_static()) {
            return m_int.g_st().hash();
        }
        const auto &dy = m_int.g_dy();
        // NOTE: mpz_size gives the number of limbs effectively used by mpz, that is,
        // the absolute value of the internal _mp_size member.
        const std::size_t size = ::mpz_size(&dy);
        if (size == 0u) {
            return 0;
        }
        // Get a read-only pointer to the limbs.
        // NOTE: there is a specific mpz_limbs_read() function
        // in later GMP versions for this.
        const ::mp_limb_t *l_ptr = dy._mp_d;
        piranha_assert(l_ptr != nullptr);
        // Init with sign.
        std::size_t retval = static_cast<std::size_t>(mpz_sgn(&dy));
        // Determine the number of std::size_t to extract.
        const std::size_t nbits_size_t = static_cast<std::size_t>(std::numeric_limits<std::size_t>::digits),
                          // NOTE: in case of overflow here, we will underestimate the number of bits
            // used in the representation of dy.
            // NOTE: use GMP_NUMB_BITS in case we are operating with a GMP lib built with nails support.
            // NOTE: GMP_NUMB_BITS is an int constant.
            tot_nbits = static_cast<std::size_t>(unsigned(GMP_NUMB_BITS) * size),
                          q = static_cast<std::size_t>(tot_nbits / nbits_size_t),
                          r = static_cast<std::size_t>(tot_nbits % nbits_size_t),
                          n_size_t = static_cast<std::size_t>(q + static_cast<std::size_t>(r != 0u));
        for (std::size_t i = 0u; i < n_size_t; ++i) {
            boost::hash_combine(
                retval, detail::read_uint<std::size_t, unsigned(GMP_LIMB_BITS - GMP_NUMB_BITS)>(l_ptr, size, i));
        }
        return retval;
    }
    /// Compute next prime number.
    /**
     * Compute the next prime number after \p this. The method uses the GMP function <tt>mpz_nextprime()</tt>.
     *
     * @return next prime.
     *
     * @throws std::invalid_argument if the sign of \p this is negative.
     */
    mp_integer nextprime() const
    {
        if (unlikely(sign() < 0)) {
            piranha_throw(std::invalid_argument, "cannot compute the next prime of a negative number");
        }
        mp_integer retval;
        retval.promote();
        auto v = get_mpz_view();
        ::mpz_nextprime(&retval.m_int.g_dy(), v);
        // NOTE: demote opportunity.
        return retval;
    }
    /// Check if \p this is a prime number
    /**
     * The method uses the GMP function <tt>mpz_probab_prime_p()</tt>.
     *
     * @param[in] reps number of primality tests to be run.
     *
     * @return 2 if \p this is definitely a prime, 1 if \p this is probably prime, 0 if \p this is definitely composite.
     *
     * @throws std::invalid_argument if \p reps is less than one or if \p this is negative.
     */
    int probab_prime_p(int reps = 25) const
    {
        if (unlikely(reps < 1)) {
            piranha_throw(std::invalid_argument, "invalid number of primality tests");
        }
        if (unlikely(sign() < 0)) {
            piranha_throw(std::invalid_argument, "cannot run primality tests on a negative number");
        }
        auto v = get_mpz_view();
        return ::mpz_probab_prime_p(v, reps);
    }
    /// Integer square root.
    /**
     * @return the truncated integer part of the square root of \p this.
     *
     * @throws std::invalid_argument if \p this is negative.
     */
    mp_integer sqrt() const
    {
        if (unlikely(sign() < 0)) {
            piranha_throw(std::invalid_argument, "cannot calculate square root of negative integer");
        }
        // NOTE: here in principle if this is static, we never need to go through dynamic
        // allocation as the sqrt <= this always.
        mp_integer retval(*this);
        if (retval.is_static()) {
            retval.promote();
        }
        ::mpz_sqrt(&retval.m_int.g_dy(), &retval.m_int.g_dy());
        return retval;
    }
    /// Factorial.
    /**
     * The GMP function <tt>mpz_fac_ui()</tt> will be used.
     *
     * @return the factorial of \p this.
     *
     * @throws std::invalid_argument if \p this is negative or larger than an implementation-defined value.
     */
    mp_integer factorial() const
    {
        if (*this > 100000L || sign() < 0) {
            piranha_throw(std::invalid_argument, "invalid input for factorial()");
        }
        // NOTE: demote opportunity.
        mp_integer retval;
        retval.promote();
        ::mpz_fac_ui(&retval.m_int.g_dy(), static_cast<unsigned long>(*this));
        return retval;
    }

private:
    template <typename T>
    static unsigned long check_choose_k(const T &k,
                                        typename std::enable_if<std::is_integral<T>::value>::type * = nullptr)
    {
        try {
            return boost::numeric_cast<unsigned long>(k);
        } catch (...) {
            piranha_throw(std::invalid_argument, "invalid k argument for binomial coefficient");
        }
    }
    template <typename T>
    static unsigned long check_choose_k(const T &k,
                                        typename std::enable_if<std::is_same<T, mp_integer>::value>::type * = nullptr)
    {
        try {
            return static_cast<unsigned long>(k);
        } catch (...) {
            piranha_throw(std::invalid_argument, "invalid k argument for binomial coefficient");
        }
    }

public:
    /// Binomial coefficient.
    /**
     * \note
     * This method is enabled only if \p T is an integral type or piranha::mp_integer.
     *
     * Will return \p this choose \p k using the GMP <tt>mpz_bin_ui</tt> function.
     *
     * @param[in] k bottom argument for the binomial coefficient.
     *
     * @return \p this choose \p k.
     *
     * @throws std::invalid_argument if \p k is outside an implementation-defined range.
     */
    template <typename T,
              typename std::enable_if<std::is_integral<T>::value || std::is_same<mp_integer, T>::value, int>::type = 0>
    mp_integer binomial(const T &k) const
    {
        if (k >= T(0)) {
            mp_integer retval(*this);
            if (is_static()) {
                retval.promote();
            }
            // NOTE: demote opportunity.
            ::mpz_bin_ui(&retval.m_int.g_dy(), &retval.m_int.g_dy(), check_choose_k(k));
            return retval;
        } else {
            // This is the case k < 0, handled according to:
            // http://arxiv.org/abs/1105.3689/
            if (sign() >= 0) {
                // n >= 0, k < 0.
                return mp_integer{0};
            } else {
                // n < 0, k < 0.
                if (k <= *this) {
                    return mp_integer{-1}.pow(*this - k) * (-mp_integer{k} - 1).binomial(*this - k);
                } else {
                    return mp_integer{0};
                }
            }
        }
    }
    /// Unitary check
    /**
     * @return \p true if \p this is equal to 1, \p false otherwise.
     */
    bool is_unitary() const
    {
        if (is_static()) {
            return m_int.g_st().is_unitary();
        }
        // NOTE: this is actually a macro.
        return mpz_cmp_ui(&m_int.g_dy(), 1ul) == 0;
    }
    /// Size in bits.
    /**
     * This method will return the number of bits necessary to represent the absolute value of \p this.
     * If \p this is zero, 1 will be returned.
     *
     * @return the size in bits of \p this.
     */
    std::size_t bits_size() const
    {
        if (is_static()) {
            constexpr auto limb_bits = detail::integer_union<NBits>::s_storage::limb_bits;
            static_assert(std::numeric_limits<std::size_t>::max() >= limb_bits * 2u, "Overflow error.");
            return static_cast<std::size_t>(m_int.g_st().bits_size());
        }
        // NOTE: in theory this could overflow. Not sure if it we should put any check here,
        // GMP does not care about overflows :(
        return ::mpz_sizeinbase(&m_int.g_dy(), 2);
    }
    /** @name Low-level interface
     * Low-level methods.
     */
    //@{
    /// Constructor from <tt>mpz_t</tt>.
    /**
     * The choice of storage type will depend on the value of \p z.
     *
     * @param[in] z <tt>mpz_t</tt> that will be used to initialise \p this.
     */
    explicit mp_integer(const ::mpz_t z)
    {
        // Get the number of limbs in use.
        const std::size_t size = ::mpz_size(z);
        // Nothing to do if z is zero.
        if (size == 0u) {
            return;
        }
        // Get a read-only pointer to the limbs of z.
        // NOTE: there is a specific mpz_limbs_read() function
        // in later GMP versions for this.
        const ::mp_limb_t *l_ptr = z->_mp_d;
        // Effective number of bits used per limb in static storage.
        const auto limb_bits = detail::integer_union<NBits>::s_storage::limb_bits;
        // Here we are checking roughly if we need static or dynamic
        // storage, based on the number of limbs used in z and the available
        // bits in static storage. It is a conservative check, meaning there
        // could be cases in which z fits in static but the check below fails
        // and forces dynamic storage.
        // Example: the mpz has a value of 1 and 64 bit limb, mp_integer
        // has 16 bit limb.
        // We could replace with mpz sizeinbase() but performance would be worse
        // probably. Need to invesitgate.
        if (unsigned(GMP_NUMB_BITS) > (limb_bits * 2u) / size) {
            promote();
            ::mpz_set(&m_int.g_dy(), z);
        } else {
            // Limb type in static storage.
            using limb_t = typename detail::integer_union<NBits>::s_storage::limb_t;
            // Number of total bits per limb in static storage (>= limb_bits).
            const auto total_bits = detail::integer_union<NBits>::s_storage::total_bits;
            // The total number of bits we will need to extract from z. We know we can compute this
            // because we know z fits static, and we can always represent the total number of bits
            // in static.
            const limb_t tot_nbits = static_cast<limb_t>(unsigned(GMP_NUMB_BITS) * size),
                         q = static_cast<limb_t>(tot_nbits / limb_bits), r = static_cast<limb_t>(tot_nbits % limb_bits),
                         n_limbs = static_cast<limb_t>(q + static_cast<limb_t>(r != 0u));
            piranha_assert(n_limbs <= 2u && n_limbs > 0u);
            // NOTE: if here the number of static limbs used will be only 1, the second
            // limb has already been zeroed out by the intial construction.
            for (std::size_t i = 0u; i < n_limbs; ++i) {
                m_int.g_st().m_limbs[i]
                    = detail::read_uint<limb_t, unsigned(GMP_LIMB_BITS - GMP_NUMB_BITS), total_bits - limb_bits>(
                        l_ptr, size, i);
            }
            // Calculate the number of limbs.
            m_int.g_st()._mp_size = m_int.g_st().calculate_n_limbs();
            // Negate if necessary.
            if (mpz_sgn(z) == -1) {
                m_int.g_st().negate();
            }
        }
    }
    /// Exact division.
    /**
     * This static method will set \p out to the quotient of \p n1 and \p n2. \p n2 must divide
     * \p n1 exactly, otherwise the behaviour will be undefined.
     *
     * @param[out] out the output value.
     * @param[in] n1 the numerator.
     * @param[in] n2 the denominator.
     *
     * @throws piranha::zero_division_error if \p n2 is zero.
     */
    static void _divexact(mp_integer &out, const mp_integer &n1, const mp_integer &n2)
    {
        if (unlikely(math::is_zero(n2))) {
            piranha_throw(zero_division_error, "division by zero");
        }
        piranha_assert(math::is_zero(n1 % n2));
        bool s0 = out.is_static(), s1 = n1.is_static(), s2 = n2.is_static();
        if (s1 && s2) {
            if (!s0) {
                // NOTE: this is safe, as out is not static while n1 and n2 are,
                // and thus out cannot overlap with n1 or n2.
                out = mp_integer{};
            }
            mp_integer r;
            // NOTE: the static division routine here does both division and remainder, we
            // need to check if there's performance to be gained by doing only the division.
            out.m_int.g_st().div(out.m_int.g_st(), r.m_int.g_st(), n1.m_int.g_st(), n2.m_int.g_st());
            return;
        }
        // out will have to be mpz in any case, promote it if needed and re-check the
        // static flags in case out coincides with n1 and/or n2.
        if (s0) {
            out.m_int.promote();
            s1 = n1.is_static();
            s2 = n2.is_static();
        }
        // NOTE: here the 0 flag means that the operand is static and needs to be promoted,
        // 1 means that it is dynamic already.
        const unsigned mask = static_cast<unsigned>(!s1) + (static_cast<unsigned>(!s2) << 1u);
        piranha_assert(mask > 0u);
        switch (mask) {
            // NOTE: case 0 here is not possible as it would mean that n1 and n2 are both static,
            // but we handled the case above.
            case 1u: {
                auto v2 = n2.m_int.g_st().get_mpz_view();
                ::mpz_divexact(&out.m_int.g_dy(), &n1.m_int.g_dy(), v2);
                break;
            }
            case 2u: {
                auto v1 = n1.m_int.g_st().get_mpz_view();
                ::mpz_divexact(&out.m_int.g_dy(), v1, &n2.m_int.g_dy());
                break;
            }
            case 3u:
                ::mpz_divexact(&out.m_int.g_dy(), &n1.m_int.g_dy(), &n2.m_int.g_dy());
        }
    }
    /// Get an non-const \p mpz_ptr to dynamic storage of \p this.
    /**
     * This method will return a pointer to the dynamic storage, mpz_struct_t. If \p this is currently stored in static
     * storage, \p this will be promoted to dynamic storage first
     *
     * Note that \p this will remain stored in dynamic storage even if after operations on \p this, value of \p this can
     * be stored in static storage.
     *
     * @return an non-const \p mpz_ptr to dynamic storage of \p this.
     */
    mpz_ptr _get_mpz_ptr()
    {
        if (is_static()) {
            promote();
        }
        return &m_int.g_dy();
    }
    //@}
private:
    // Safely compute the absolute value of an mpz_size_t. For use in serialization.
    static detail::mpz_size_t safe_abs_size(detail::mpz_size_t s)
    {
        if (unlikely(s < -detail::safe_abs_sint<detail::mpz_size_t>::value)) {
            piranha_throw(std::overflow_error, "the number of limbs is too large");
        }
        return static_cast<detail::mpz_size_t>(s >= 0 ? s : -s);
    }
    // Enablers.
    template <typename U>
    using boost_save_binary_enabler
        = enable_if_t<conjunction<has_boost_save<boost::archive::binary_oarchive, bool>,
                                  has_boost_save<boost::archive::binary_oarchive,
                                                 decltype(std::declval<U>()._mp_alloc)>,
                                  has_boost_save<boost::archive::binary_oarchive, decltype(std::declval<U>()._mp_size)>,
                                  has_boost_save<boost::archive::binary_oarchive,
                                                 typename std::remove_pointer<decltype(std::declval<U>()._mp_d)>::type>,
                                  has_boost_save<boost::archive::binary_oarchive,
                                                 typename detail::integer_union<NBits>::s_storage::limb_t>>::value,
                      int>;
    template <typename U>
    using boost_load_binary_enabler
        = enable_if_t<conjunction<has_boost_load<boost::archive::binary_iarchive, bool>,
                                  has_boost_load<boost::archive::binary_iarchive,
                                                 decltype(std::declval<U>()._mp_alloc)>,
                                  has_boost_load<boost::archive::binary_iarchive, decltype(std::declval<U>()._mp_size)>,
                                  has_boost_load<boost::archive::binary_iarchive,
                                                 typename std::remove_pointer<decltype(std::declval<U>()._mp_d)>::type>,
                                  has_boost_load<boost::archive::binary_iarchive,
                                                 typename detail::integer_union<NBits>::s_storage::limb_t>>::value,
                      int>;

public:
    /// Save to a Boost binary archive.
    /**
     * \note
     * This method is enabled only if all the fundamental types describing the internal state of the integer satisfy
     * piranha::has_boost_save.
     *
     * This method will serialize \p this into \p ar.
     *
     * @param[in] ar target archive.
     *
     * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
     * @throws unspecified any exception thrown by piranha::boost_save().
     */
    template <typename U = detail::mpz_struct_t, boost_save_binary_enabler<U> = 0>
    void boost_save(boost::archive::binary_oarchive &ar) const
    {
        if (is_static()) {
            piranha::boost_save(ar, true);
            // NOTE: alloc size is known for static ints.
            const auto size = m_int.g_st()._mp_size;
            piranha::boost_save(ar, size);
            std::for_each(m_int.g_st().m_limbs.data(), m_int.g_st().m_limbs.data() + (size >= 0 ? size : -size),
                          [&ar](const typename detail::integer_union<NBits>::s_storage::limb_t &l) {
                              piranha::boost_save(ar, l);
                          });
        } else {
            piranha::boost_save(ar, false);
            // NOTE: don't record alloc size, we will reserve an adequate size on load.
            piranha::boost_save(ar, m_int.g_dy()._mp_size);
            std::for_each(m_int.g_dy()._mp_d, m_int.g_dy()._mp_d + safe_abs_size(m_int.g_dy()._mp_size),
                          [&ar](const ::mp_limb_t &l) { piranha::boost_save(ar, l); });
        }
    }
#define PIRANHA_MP_INTEGER_BOOST_S11N_LATEST_VERSION 0u
    /// Save to a Boost text archive.
    /**
     * This method will serialize \p this into \p ar as a string in decimal format.
     *
     * @param[in] ar target archive.
     *
     * @throws unspecified any exception thrown by piranha::boost_save() or by the
     * conversion of \p this to string.
     */
    void boost_save(boost::archive::text_oarchive &ar) const
    {
        // Save the version.
        piranha::boost_save(ar, PIRANHA_MP_INTEGER_BOOST_S11N_LATEST_VERSION);
        // NOTE: this requires too many allocations, it should be refactored together
        // with the mpz streaming so that we just need a single string.
        std::ostringstream oss;
        oss << *this;
        piranha::boost_save(ar, oss.str());
    }
    /// Deserialize from Boost binary archive.
    /**
     * \note
     * This method is enabled only if all the fundamental types describing the internal state of the integer satisfy
     * piranha::has_boost_load.
     *
     * This method will deserialize from \p ar into \p this. The method offers the basic exception guarantee
     * and performs minimal checking of the input data. Calling this method will result in undefined behaviour
     * if \p ar does not contain an integer serialized via boost_save().
     *
     * @param[in] ar the source archive.
     *
     * @throws std::invalid_argument if the serialized integer is static and its number of limbs is greater than 2.
     * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
     * @throws unspecified any exception thrown by piranha::boost_load().
     */
    template <typename U = detail::mpz_struct_t, boost_load_binary_enabler<U> = 0>
    void boost_load(boost::archive::binary_iarchive &ar)
    {
        const bool this_s = is_static();
        bool s;
        piranha::boost_load(ar, s);
        // If the staticness of this and the serialized object differ, we have
        // to adjust this.
        if (s != this_s) {
            if (this_s) {
                // This is static, serialized is dynamic.
                promote();
            } else {
                // This is dynamic, serialized is static.
                *this = mp_integer{};
            }
        }
        if (s) {
            try {
                // NOTE: alloc is already set to the correct value.
                piranha_assert(m_int.g_st()._mp_alloc == -1);
                piranha::boost_load(ar, m_int.g_st()._mp_size);
                const auto size = safe_abs_size(m_int.g_st()._mp_size);
                if (unlikely(size > 2)) {
                    piranha_throw(std::invalid_argument,
                                  "cannot deserialize a static integer with " + std::to_string(size) + " limbs");
                }
                auto data = m_int.g_st().m_limbs.data();
                std::for_each(data, data + size, [&ar](typename detail::integer_union<NBits>::s_storage::limb_t &l) {
                    piranha::boost_load(ar, l);
                });
                // Zero the limbs that were not loaded from the archive.
                std::fill(data + size, data + 2, 0u);
            } catch (...) {
                // Reset the static before re-throwing.
                m_int.g_st()._mp_size = 0;
                m_int.g_st().m_limbs[0] = 0u;
                m_int.g_st().m_limbs[1] = 0u;
                throw;
            }
        } else {
            detail::mpz_size_t sz;
            piranha::boost_load(ar, sz);
            const auto size = safe_abs_size(sz);
            ::_mpz_realloc(&m_int.g_dy(), size);
            try {
                std::for_each(m_int.g_dy()._mp_d, m_int.g_dy()._mp_d + size,
                              [&ar](::mp_limb_t &l) { piranha::boost_load(ar, l); });
                m_int.g_dy()._mp_size = sz;
            } catch (...) {
                // NOTE: only possible exception here is when boost_load(ar,l) throws. In this case we have successfully
                // reallocated and possibly written some limbs, but we have not set the size yet. Just zero out the mpz.
                ::mpz_set_ui(&m_int.g_dy(), 0u);
                throw;
            }
        }
    }
    /// Deserialize from Boost text archive.
    /**
     * This method will deserialize from \p ar into \p this.
     *
     * @param[in] ar the source archive.
     *
     * @throws unspecified any exception thrown by piranha::boost_load(), or by the constructor of
     * piranha::mp_integer from string.
     */
    void boost_load(boost::archive::text_iarchive &ar)
    {
        unsigned version;
        piranha::boost_load(ar, version);
        if (unlikely(version > PIRANHA_MP_INTEGER_BOOST_S11N_LATEST_VERSION)) {
            piranha_throw(std::invalid_argument, "the mp_integer Boost archive version " + std::to_string(version)
                                                     + " is greater than the latest archive version "
                                                     + std::to_string(PIRANHA_MP_INTEGER_BOOST_S11N_LATEST_VERSION)
                                                     + " supported by this version of Piranha");
        }
        PIRANHA_MAYBE_TLS std::string tmp;
        piranha::boost_load(ar, tmp);
        *this = mp_integer{tmp};
    }
#undef PIRANHA_MP_INTEGER_BOOST_S11N_LATEST_VERSION
#if defined(PIRANHA_WITH_MSGPACK)
private:
    // msgpack enabler.
    template <typename Stream>
    using msgpack_pack_enabler =
        // NOTE: integral types are always serializable for any valid stream.
        typename std::enable_if<is_msgpack_stream<Stream>::value, int>::type;

public:
    /// Pack in msgpack format.
    /**
     * \note
     * This method is enabled only if \p Stream satisfies piranha::is_msgpack_stream.
     *
     * This method will pack \p this into \p p. If \p f is msgpack_format::portable, then
     * a decimal string representation of \p this is packed. Otherwise, an array of 3 elements
     * is packed: the first element is a boolean representing whether the integer is stored in static
     * storage or not, the second element is a boolean representing the sign of the integer (\p true
     * for positive or zero, \p false for negative) and the last element is an array of limbs.
     *
     * @param[in] p target <tt>msgpack::packer</tt>.
     * @param[in] f the desired piranha::msgpack_format.
     *
     * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::packer</tt>,
     * - piranha::msgpack_pack(),
     * - the conversion of \p this to string.
     */
    template <typename Stream, msgpack_pack_enabler<Stream> = 0>
    void msgpack_pack(msgpack::packer<Stream> &p, msgpack_format f) const
    {
        if (f == msgpack_format::binary) {
            // Regardless of format, we always pack an array of 3 elements:
            // - staticness,
            // - sign of size,
            // - array of limbs.
            p.pack_array(3);
            if (is_static()) {
                const auto size = m_int.g_st()._mp_size;
                // No problems with the static casting here, abs(size) is at most 2.
                const auto usize = static_cast<std::uint32_t>(size >= 0 ? size : -size);
                piranha::msgpack_pack(p, true, f);
                // Store whether the size is positive (true) or negative (false).
                piranha::msgpack_pack(p, size >= 0, f);
                p.pack_array(usize);
                std::for_each(m_int.g_st().m_limbs.data(), m_int.g_st().m_limbs.data() + usize,
                              [&p, f](const typename detail::integer_union<NBits>::s_storage::limb_t &l) {
                                  piranha::msgpack_pack(p, l, f);
                              });
            } else {
                const auto size = m_int.g_dy()._mp_size;
                std::uint32_t usize;
                try {
                    usize = boost::numeric_cast<std::uint32_t>(safe_abs_size(size));
                } catch (...) {
                    piranha_throw(std::overflow_error, "the number of limbs is too large");
                }
                piranha::msgpack_pack(p, false, f);
                piranha::msgpack_pack(p, size >= 0, f);
                p.pack_array(usize);
                std::for_each(m_int.g_dy()._mp_d, m_int.g_dy()._mp_d + usize,
                              [&p, f](const ::mp_limb_t &l) { piranha::msgpack_pack(p, l, f); });
            }
        } else {
            std::ostringstream oss;
            oss << *this;
            piranha::msgpack_pack(p, oss.str(), f);
        }
    }
    /// Convert from msgpack object.
    /**
     * This method will convert the object \p o into \p this. If \p f is piranha::msgpack_format::binary,
     * this method offers the basic exception safety guarantee and it performs minimal checking on the input data.
     * Calling this method in binary mode will result in undefined behaviour if \p o does not contain an integer
     * serialized via msgpack_pack().
     *
     * @param[in] o source object.
     * @param[in] f the desired piranha::msgpack_format.
     *
     * @throws msgpack::type_error if, in binary mode, the serialized object is not an array of 3 elements.
     * @throws std::invalid_argument if, in binary mode, the serialized static integer has a number of limbs
     * greater than 2.
     * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert(),
     * - the constructor of piranha::mp_integer from string.
     */
    void msgpack_convert(const msgpack::object &o, msgpack_format f)
    {
        if (f == msgpack_format::binary) {
            PIRANHA_MAYBE_TLS std::vector<msgpack::object> vobj;
            o.convert(vobj);
            // The serialized object needs to be a triple.
            if (unlikely(vobj.size() != 3u)) {
                piranha_throw(msgpack::type_error, );
            }
            // Get the staticness of the serialized object.
            bool s;
            piranha::msgpack_convert(s, vobj[0u], f);
            // Bring this into the same staticness as the serialized object.
            const auto this_s = is_static();
            if (s != this_s) {
                if (this_s) {
                    promote();
                } else {
                    *this = mp_integer{};
                }
            }
            // Get the size sign.
            bool size_sign;
            piranha::msgpack_convert(size_sign, vobj[1u], f);
            PIRANHA_MAYBE_TLS std::vector<msgpack::object> vlimbs;
            // Get the limbs.
            vobj[2u].convert(vlimbs);
            const auto size = vlimbs.size();
            // We will need to iterate over the vector of limbs in both cases.
            auto it = vlimbs.begin();
            if (s) {
                if (unlikely(size > 2u)) {
                    piranha_throw(std::invalid_argument, "cannot deserialize a static integer with "
                                                             + std::to_string(vlimbs.size()) + " limbs");
                }
                try {
                    // Fill in the limbs.
                    auto data = m_int.g_st().m_limbs.data();
                    // NOTE: for_each is guaranteed to proceed in order, so we are sure that it and l are consistent.
                    std::for_each(data, data + size,
                                  [f, &it](typename detail::integer_union<NBits>::s_storage::limb_t &l) {
                                      piranha::msgpack_convert(l, *it, f);
                                      ++it;
                                  });
                    // Zero the limbs that were not loaded from the archive.
                    std::fill(data + size, data + 2, 0u);
                    // Set the size, with sign.
                    m_int.g_st()._mp_size = static_cast<detail::mpz_size_t>(
                        size_sign ? static_cast<detail::mpz_size_t>(size) : -static_cast<detail::mpz_size_t>(size));
                } catch (...) {
                    // Reset the static before re-throwing.
                    m_int.g_st()._mp_size = 0;
                    m_int.g_st().m_limbs[0] = 0u;
                    m_int.g_st().m_limbs[1] = 0u;
                    throw;
                }
            } else {
                detail::mpz_size_t sz;
                try {
                    sz = boost::numeric_cast<detail::mpz_size_t>(size);
                    // We need to be able to negate this.
                    if (unlikely(sz > detail::safe_abs_sint<detail::mpz_size_t>::value)) {
                        throw std::overflow_error("");
                    }
                } catch (...) {
                    piranha_throw(std::overflow_error, "the number of limbs is too large");
                }
                ::_mpz_realloc(&m_int.g_dy(), sz);
                try {
                    std::for_each(m_int.g_dy()._mp_d, m_int.g_dy()._mp_d + sz, [f, &it](::mp_limb_t &l) {
                        piranha::msgpack_convert(l, *it, f);
                        ++it;
                    });
                    m_int.g_dy()._mp_size = static_cast<detail::mpz_size_t>(size_sign ? sz : -sz);
                } catch (...) {
                    ::mpz_set_ui(&m_int.g_dy(), 0u);
                    throw;
                }
            }
        } else {
            PIRANHA_MAYBE_TLS std::string tmp;
            piranha::msgpack_convert(tmp, o, f);
            *this = mp_integer(tmp);
        }
    }
#endif

private:
    detail::integer_union<NBits> m_int;
};

/// Alias for piranha::mp_integer with default bit size.
using integer = mp_integer<>;

namespace detail
{

// Temporary TMP structure to detect mp_integer types.
// Should be replaced by is_instance_of once (or if) we move
// from NBits to an integral_constant for selecting limb
// size in mp_integer.
template <typename T>
struct is_mp_integer : std::false_type {
};

template <int NBits>
struct is_mp_integer<mp_integer<NBits>> : std::true_type {
};
}

namespace math
{

/// Specialisation of the implementation of piranha::math::multiply_accumulate() for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct multiply_accumulate_impl<T, T, T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * This implementation will use piranha::mp_integer::multiply_accumulate().
     *
     * @param[in,out] x target value for accumulation.
     * @param[in] y first argument.
     * @param[in] z second argument.
     */
    void operator()(T &x, const T &y, const T &z) const
    {
        x.multiply_accumulate(y, z);
    }
};

/// Specialisation of the piranha::math::negate() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct negate_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * Will use internally piranha::mp_integer::negate().
     *
     * @param[in,out] n piranha::mp_integer to be negated.
     */
    void operator()(T &n) const
    {
        n.negate();
    }
};

/// Specialisation of the piranha::math::is_zero() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct is_zero_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * Will use internally piranha::mp_integer::sign().
     *
     * @param[in] n piranha::mp_integer to be tested.
     *
     * @return \p true if \p n is zero, \p false otherwise.
     */
    bool operator()(const T &n) const
    {
        return n.sign() == 0;
    }
};

/// Specialisation of the piranha::math::is_unitary() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct is_unitary_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * Will use internally piranha::mp_integer::is_unitary().
     *
     * @param[in] n piranha::mp_integer to be tested.
     *
     * @return \p true if \p n is equal to 1, \p false otherwise.
     */
    bool operator()(const T &n) const
    {
        return n.is_unitary();
    }
};

/// Specialisation of the piranha::math::abs() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct abs_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[in] n input parameter.
     *
     * @return absolute value of \p n.
     */
    T operator()(const T &n) const
    {
        return n.abs();
    }
};

/// Specialisation of the piranha::math::sin() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct sin_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[in] n argument.
     *
     * @return sine of \p n.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    T operator()(const T &n) const
    {
        if (is_zero(n)) {
            return T(0);
        }
        piranha_throw(std::invalid_argument, "cannot compute the sine of a non-zero integer");
    }
};

/// Specialisation of the piranha::math::cos() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct cos_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[in] n argument.
     *
     * @return cosine of \p n.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    T operator()(const T &n) const
    {
        if (is_zero(n)) {
            return T(1);
        }
        piranha_throw(std::invalid_argument, "cannot compute the cosine of a non-zero integer");
    }
};

/// Specialisation of the piranha::math::partial() functor for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct partial_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @return an instance of piranha::mp_integer constructed from zero.
     */
    T operator()(const T &, const std::string &) const
    {
        return T{};
    }
};

/// Factorial.
/**
 * @param[in] n factorial argument.
 *
 * @return the output of piranha::mp_integer::factorial().
 *
 * @throws unspecified any exception thrown by piranha::mp_integer::factorial().
 */
template <int NBits>
inline mp_integer<NBits> factorial(const mp_integer<NBits> &n)
{
    return n.factorial();
}

/// Default functor for the implementation of piranha::math::ipow_subs().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename U, typename = void>
struct ipow_subs_impl {
};

/// Substitution of integral power.
/**
 * Substitute the integral power of a symbolic variable with a generic object.
 * The actual implementation of this function is in the piranha::math::ipow_subs_impl functor.
 *
 * @param[in] x quantity that will be subject to substitution.
 * @param[in] name name of the symbolic variable that will be substituted.
 * @param[in] n power of \p name that will be substituted.
 * @param[in] y object that will substitute the variable.
 *
 * @return \p x after substitution  of \p name to the power of \p n with \p y.
 *
 * @throws unspecified any exception thrown by the call operator of piranha::math::subs_impl.
 */
template <typename T, typename U>
inline auto ipow_subs(const T &x, const std::string &name, const integer &n, const U &y)
    -> decltype(ipow_subs_impl<T, U>()(x, name, n, y))
{
    return ipow_subs_impl<T, U>()(x, name, n, y);
}

/// Specialisation of the piranha::math::add3() functor for piranha::mp_integer.
/**
 * This specialisation is activated when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct add3_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[out] out the output value.
     * @param[in] a the first operand.
     * @param[in] b the second operand.
     *
     * @return the output of piranha::mp_integer::add().
     */
    auto operator()(T &out, const T &a, const T &b) const -> decltype(out.add(a, b))
    {
        return out.add(a, b);
    }
};

/// Specialisation of the piranha::math::sub3() functor for piranha::mp_integer.
/**
 * This specialisation is activated when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct sub3_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[out] out the output value.
     * @param[in] a the first operand.
     * @param[in] b the second operand.
     *
     * @return the output of piranha::mp_integer::sub().
     */
    auto operator()(T &out, const T &a, const T &b) const -> decltype(out.sub(a, b))
    {
        return out.sub(a, b);
    }
};

/// Specialisation of the piranha::math::mul3() functor for piranha::mp_integer.
/**
 * This specialisation is activated when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct mul3_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[out] out the output value.
     * @param[in] a the first operand.
     * @param[in] b the second operand.
     *
     * @return the output of piranha::mp_integer::mul().
     */
    auto operator()(T &out, const T &a, const T &b) const -> decltype(out.mul(a, b))
    {
        return out.mul(a, b);
    }
};

/// Specialisation of the piranha::math::div3() functor for piranha::mp_integer.
/**
 * This specialisation is activated when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct div3_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * @param[out] out the output value.
     * @param[in] a the first operand.
     * @param[in] b the second operand.
     *
     * @return the output of piranha::mp_integer::div().
     */
    auto operator()(T &out, const T &a, const T &b) const -> decltype(out.div(a, b))
    {
        return out.div(a, b);
    }
};

/// Implementation of piranha::math::divexact() for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct divexact_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * The call operator will first determine quotient and remainder via piranha::mp_integer::divrem().
     * If the remainder is not null, an error will be thrown.
     *
     * @param[out] out return value.
     * @param[in] a first argument.
     * @param[in] b second argument.
     *
     * @return a reference to \p out.
     *
     * @throws piranha::math::inexact_division if the division of \p a by \p b is not exact.
     * @throws unspecified any exception thrown by piranha::mp_integer::divrem().
     */
    T &operator()(T &out, const T &a, const T &b) const
    {
        T r;
        T::divrem(out, r, a, b);
        if (!is_zero(r)) {
            piranha_throw(inexact_division, );
        }
        return out;
    }
};
}

namespace detail
{

// Enabler for the GCD specialisation.
template <typename T, typename U>
using mp_integer_gcd_enabler = typename std::enable_if<(std::is_integral<T>::value && is_mp_integer<U>::value)
                                                       || (std::is_integral<U>::value && is_mp_integer<T>::value)
                                                       || (is_mp_integer<T>::value && is_mp_integer<U>::value)>::type;
}

namespace math
{

/// Implementation of piranha::math::gcd() for piranha::mp_integer.
/**
 * This specialisation is enabled when:
 * - \p T and \p U are both instances of piranha::mp_integer,
 * - \p T is an instance of piranha::mp_integer and \p U is an integral type,
 * - \p U is an instance of piranha::mp_integer and \p T is an integral type.
 *
 * The result will be calculated via piranha::mp_integer::gcd(), after any necessary type conversion.
 */
template <typename T, typename U>
struct gcd_impl<T, U, detail::mp_integer_gcd_enabler<T, U>> {
    /// Call operator, piranha::mp_integer - piranha::mp_integer overload.
    /**
     * @param[in] a first argument.
     * @param[in] b second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <int NBits>
    mp_integer<NBits> operator()(const mp_integer<NBits> &a, const mp_integer<NBits> &b) const
    {
        mp_integer<NBits> retval;
        mp_integer<NBits>::gcd(retval, a, b);
        return retval;
    }
    /// Call operator, piranha::mp_integer - integral overload.
    /**
     * @param[in] a first argument.
     * @param[in] b second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <int NBits, typename T1>
    mp_integer<NBits> operator()(const mp_integer<NBits> &a, const T1 &b) const
    {
        return operator()(a, mp_integer<NBits>(b));
    }
    /// Call operator, integral - piranha::mp_integer overload.
    /**
     * @param[in] a first argument.
     * @param[in] b second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <int NBits, typename T1>
    mp_integer<NBits> operator()(const T1 &a, const mp_integer<NBits> &b) const
    {
        return operator()(b, a);
    }
};

/// Implementation of piranha::math::gcd3() for piranha::mp_integer.
/**
 * This specialisation is enabled when \p T is an instances of piranha::mp_integer.
 */
template <typename T>
struct gcd3_impl<T, typename std::enable_if<detail::is_mp_integer<T>::value>::type> {
    /// Call operator.
    /**
     * This call operator will use internally piranha::mp_integer::gcd().
     *
     * @param[out] out return value.
     * @param[in] a first argument.
     * @param[in] b second argument.
     *
     * @return a reference to \p out.
     */
    template <int NBits>
    mp_integer<NBits> &operator()(mp_integer<NBits> &out, const mp_integer<NBits> &a, const mp_integer<NBits> &b) const
    {
        mp_integer<NBits>::gcd(out, a, b);
        return out;
    }
};
}

namespace detail
{

// Enabler for the overload below.
template <typename Int>
using ipow_subs_int_enabler = typename std::enable_if<std::is_integral<Int>::value, int>::type;
}

namespace math
{

/// Substitution of integral power (convenience overload).
/**
 * \note
 * This function is enabled only if \p Int is a C++ integral type.
 *
 * This function is a convenience wrapper that will call the other piranha::math::ipow_subs() overload, with \p n
 * converted to a piranha::integer.
 *
 * @param[in] x quantity that will be subject to substitution.
 * @param[in] name name of the symbolic variable that will be substituted.
 * @param[in] n power of \p name that will be substituted.
 * @param[in] y object that will substitute the variable.
 *
 * @return \p x after substitution  of \p name to the power of \p n with \p y.
 *
 * @throws unspecified any exception thrown by the other overload of piranha::math::ipow_subs() or by the construction
 * of piranha::integer from an integral value.
 */
template <typename T, typename U, typename Int, detail::ipow_subs_int_enabler<Int> = 0>
inline auto ipow_subs(const T &x, const std::string &name, const Int &n, const U &y)
    -> decltype(ipow_subs(x, name, integer(n), y))
{
    return ipow_subs(x, name, integer(n), y);
}
}

/// Type trait to detect the presence of the piranha::math::ipow_subs function.
/**
 * The type trait will be \p true if piranha::math::ipow_subs can be successfully called on instances
 * of type \p T, with an instance of type \p U as substitution argument.
 */
template <typename T, typename U>
class has_ipow_subs : detail::sfinae_types
{
    typedef typename std::decay<T>::type Td;
    typedef typename std::decay<U>::type Ud;
    template <typename T1, typename U1>
    static auto test(const T1 &t, const U1 &u)
        -> decltype(math::ipow_subs(t, std::declval<std::string const &>(), std::declval<integer const &>(), u), void(),
                    yes());
    static no test(...);

public:
    /// Value of the type trait.
    static const bool value = std::is_same<decltype(test(std::declval<Td>(), std::declval<Ud>())), yes>::value;
};

template <typename T, typename U>
const bool has_ipow_subs<T, U>::value;

/// Type trait to detect the presence of the integral power substitution method in keys.
/**
 * This type trait will be \p true if the decay type of \p Key provides a const method <tt>ipow_subs()</tt> accepting as
 * const parameters a string,
 * an instance of piranha::integer, an instance of \p T and an instance of piranha::symbol_set. The return value of the
 * method must be an <tt>std::vector</tt>
 * of pairs in which the second type must be \p Key itself. The <tt>ipow_subs()</tt> method represents the substitution
 * of the integral power of a symbol with
 * an instance of type \p T.
 *
 * The decay type of \p Key must satisfy piranha::is_key.
 */
template <typename Key, typename T>
class key_has_ipow_subs : detail::sfinae_types
{
    typedef typename std::decay<Key>::type Keyd;
    typedef typename std::decay<T>::type Td;
    PIRANHA_TT_CHECK(is_key, Keyd);
    template <typename Key1, typename T1>
    static auto test(const Key1 &k, const T1 &t)
        -> decltype(k.ipow_subs(std::declval<const std::string &>(), std::declval<const integer &>(), t,
                                std::declval<const symbol_set &>()));
    static no test(...);
    template <typename T1>
    struct check_result_type {
        static const bool value = false;
    };
    template <typename Res>
    struct check_result_type<std::vector<std::pair<Res, Keyd>>> {
        static const bool value = true;
    };

public:
    /// Value of the type trait.
    static const bool value = check_result_type<decltype(test(std::declval<Keyd>(), std::declval<Td>()))>::value;
};

// Static init.
template <typename Key, typename T>
const bool key_has_ipow_subs<Key, T>::value;

/// Specialisation of piranha::has_exact_ring_operations for piranha::mp_integer.
/**
 * This specialisation is enabled if the decay type of \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct has_exact_ring_operations<T, typename std::
                                        enable_if<detail::is_mp_integer<typename std::decay<T>::type>::value>::type> {
    /// Value of the type trait.
    static const bool value = true;
};

template <typename T>
const bool has_exact_ring_operations<T, typename std::enable_if<detail::is_mp_integer<
                                            typename std::decay<T>::type>::value>::type>::value;

inline namespace literals
{

/// Literal for arbitrary-precision integers.
/**
 * @param[in] s literal string.
 *
 * @return a piranha::mp_integer constructed from \p s.
 *
 * @throws unspecified any exception thrown by the constructor of
 * piranha::mp_integer from string.
 */
inline integer operator"" _z(const char *s)
{
    return integer(s);
}
}

inline namespace impl
{

template <typename Archive, typename T>
using mp_integer_boost_save_enabler = typename std::
    enable_if<conjunction<detail::is_mp_integer<T>, is_detected<boost_save_member_t, Archive, T>>::value>::type;

template <typename Archive, typename T>
using mp_integer_boost_load_enabler = typename std::
    enable_if<conjunction<detail::is_mp_integer<T>, is_detected<boost_load_member_t, Archive, T>>::value>::type;
}

/// Implementation of piranha::boost_save() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled if \p T is an instance of piranha::mp_integer supporting
 * the piranha::mp_integer::boost_save() method with an archive of type \p Archive.
 */
template <typename Archive, typename T>
class boost_save_impl<Archive, T, mp_integer_boost_save_enabler<Archive, T>>
{
public:
    /// Call operator.
    /**
     * The call operator will invoke piranha::mp_integer::boost_save().
     *
     * @param[in] ar target archive.
     * @param[in] n piranha::mp_integer to be serialized into \p ar.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::boost_save().
     */
    void operator()(Archive &ar, const T &n) const
    {
        n.boost_save(ar);
    }
};

/// Implementation of piranha::boost_load() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled if \p T is an instance of piranha::mp_integer supporting
 * the piranha::mp_integer::boost_load() method with an archive of type \p Archive.
 */
template <typename Archive, typename T>
class boost_load_impl<Archive, T, mp_integer_boost_load_enabler<Archive, T>>
{
public:
    /// Call operator.
    /**
     * The call operator will invoke piranha::mp_integer::boost_load().
     *
     * @param[in] ar source archive.
     * @param[in] n target piranha::mp_integer.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::boost_load().
     */
    void operator()(Archive &ar, T &n) const
    {
        n.boost_load(ar);
    }
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

// Enablers for msgpack serialization.
template <typename Stream, typename T>
using mp_integer_msgpack_pack_enabler = typename std::
    enable_if<conjunction<detail::is_mp_integer<T>, is_detected<msgpack_pack_member_t, Stream, T>>::value>::type;

template <typename T>
using mp_integer_msgpack_convert_enabler = typename std::
    enable_if<conjunction<detail::is_mp_integer<T>, is_detected<msgpack_convert_member_t, T>>::value>::type;
}

/// Implementation of piranha::msgpack_pack() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled if \p T is an instance of piranha::mp_integer supporting
 * the piranha::mp_integer::msgpack_pack() method with a stream of type \p Stream.
 */
template <typename Stream, typename T>
struct msgpack_pack_impl<Stream, T, mp_integer_msgpack_pack_enabler<Stream, T>> {
    /// Call operator.
    /**
     * The call operator will use piranha::mp_integer::msgpack_pack() internally.
     *
     * @param[in] p target <tt>msgpack::packer</tt>.
     * @param[in] n piranha::mp_integer to be serialized.
     * @param[in] f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::msgpack_pack().
     */
    void operator()(msgpack::packer<Stream> &p, const T &n, msgpack_format f) const
    {
        n.msgpack_pack(p, f);
    }
};

/// Implementation of piranha::msgpack_convert() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled if \p T is an instance of piranha::mp_integer.
 */
template <typename T>
struct msgpack_convert_impl<T, mp_integer_msgpack_convert_enabler<T>> {
    /// Call operator.
    /**
     * The call operator will use piranha::mp_integer::msgpack_convert() internally.
     *
     * @param[in] n target piranha::mp_integer.
     * @param[in] o the <tt>msgpack::object</tt> to be converted into \p n.
     * @param[in] f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::msgpack_convert().
     */
    void operator()(T &n, const msgpack::object &o, msgpack_format f) const
    {
        n.msgpack_convert(o, f);
    }
};

#endif
}

namespace std
{

/// Specialisation of \p std::hash for piranha::mp_integer.
template <int NBits>
struct hash<piranha::mp_integer<NBits>> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::mp_integer<NBits> argument_type;
    /// Hash operator.
    /**
     * @param[in] n piranha::mp_integer whose hash value will be returned.
     *
     * @return <tt>n.hash()</tt>.
     *
     * @see piranha::mp_integer::hash()
     */
    result_type operator()(const argument_type &n) const
    {
        return n.hash();
    }
};
}

#endif
