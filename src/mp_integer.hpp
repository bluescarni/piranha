/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PIRANHA_MP_INTEGER_HPP
#define PIRANHA_MP_INTEGER_HPP

#include <algorithm>
#include <array>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <cstdint>
#include <gmp.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "small_vector.hpp"

namespace piranha { namespace detail {

// mpz_t is an array of some struct.
using mpz_struct_t = std::remove_extent< ::mpz_t>::type;
// Integral types used for allocation size and number of limbs.
using mpz_alloc_t = decltype(std::declval<mpz_struct_t>()._mp_alloc);
using mpz_size_t = decltype(std::declval<mpz_struct_t>()._mp_size);

// Some misc tests to check that the mpz struct conforms to our expectations.
// This is crucial for the implementation of the union integer type.
struct expected_mpz_struct_t
{
	mpz_alloc_t	_mp_alloc;
	mpz_size_t	_mp_size;
	::mp_limb_t	*_mp_d;
};

static_assert(sizeof(expected_mpz_struct_t) == sizeof(mpz_struct_t) &&
	std::is_standard_layout<mpz_struct_t>::value && std::is_standard_layout<expected_mpz_struct_t>::value &&
	offsetof(mpz_struct_t,_mp_alloc) == 0u &&
	offsetof(mpz_struct_t,_mp_size) == offsetof(expected_mpz_struct_t,_mp_size) &&
	offsetof(mpz_struct_t,_mp_d) == offsetof(expected_mpz_struct_t,_mp_d) &&
	std::is_same<mpz_alloc_t,decltype(std::declval<mpz_struct_t>()._mp_alloc)>::value &&
	std::is_same<mpz_size_t,decltype(std::declval<mpz_struct_t>()._mp_size)>::value &&
	std::is_same< ::mp_limb_t *,decltype(std::declval<mpz_struct_t>()._mp_d)>::value,
	"Invalid mpz_t struct layout.");

// Metaprogramming to select the limb/dlimb types.
template <int NBits>
struct si_limb_types
{
	static_assert(NBits == 0,"Invalid limb size.");
};

#if defined(PIRANHA_UINT128_T)
template <>
struct si_limb_types<64>
{
	using limb_t = std::uint_least64_t;
	using dlimb_t = PIRANHA_UINT128_T;
	static_assert(static_cast<dlimb_t>(boost::integer_traits<limb_t>::const_max) <=
		-dlimb_t(1) / boost::integer_traits<limb_t>::const_max,"128-bit integer is too narrow.");
	static const limb_t limb_bits = 64;
};
#endif

template <>
struct si_limb_types<32>
{
	using limb_t = std::uint_least32_t;
	using dlimb_t = std::uint_least64_t;
	static const limb_t limb_bits = 32;
};

template <>
struct si_limb_types<16>
{
	using limb_t = std::uint_least16_t;
	using dlimb_t = std::uint_least32_t;
	static const limb_t limb_bits = 16;
};

template <>
struct si_limb_types<8>
{
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
	>
{};

// Simple RAII holder for GMP integers.
struct mpz_raii
{
	mpz_raii()
	{
		::mpz_init(&m_mpz);
		piranha_assert(m_mpz._mp_alloc > 0);
	}
	mpz_raii(const mpz_raii &) = delete;
	mpz_raii(mpz_raii &&) = delete;
	mpz_raii &operator=(const mpz_raii &) = delete;
	mpz_raii &operator=(mpz_raii &&) = delete;
	~mpz_raii() noexcept
	{
		if (m_mpz._mp_d != nullptr) {
			::mpz_clear(&m_mpz);
		}
	}
	mpz_struct_t m_mpz;
};

static inline std::ostream &stream_mpz(std::ostream &os, const mpz_struct_t &mpz)
{
	const std::size_t size_base10 = ::mpz_sizeinbase(&mpz,10);
	if (unlikely(size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2))) {
		piranha_throw(std::overflow_error,"number of digits is too large");
	}
	const auto total_size = size_base10 + 2u;
	small_vector<char> tmp;
	tmp.resize(static_cast<small_vector<char>::size_type>(total_size));
	if (unlikely(tmp.size() != total_size)) {
		piranha_throw(std::overflow_error,"number of digits is too large");
	}
	os << ::mpz_get_str(&tmp[0u],10,&mpz);
	return os;
}

template <int NBits>
struct static_integer
{
	using dlimb_t = typename si_limb_types<NBits>::dlimb_t;
	using limb_t = typename si_limb_types<NBits>::limb_t;
	// Limb bits used for the representation of the number.
	static const limb_t limb_bits = si_limb_types<NBits>::limb_bits;
	// Total number of bits in the limb type, >= limb_bits.
	static const unsigned total_bits = static_cast<unsigned>(std::numeric_limits<limb_t>::digits);
	static_assert(total_bits >= limb_bits,"Invalid limb_t type.");
	using limbs_type = std::array<limb_t,std::size_t(2)>;
	// Check: we need to be able to address all bits in the 2 limbs using limb_t.
	static_assert(limb_bits < boost::integer_traits<limb_t>::const_max / 2u,"Overflow error.");
	// NOTE: init everything otherwise zero is gonna be represented by undefined values in lo/hi.
	static_integer():_mp_alloc(0),_mp_size(0),m_limbs() {}
	template <typename Integer, typename = typename std::enable_if<std::is_integral<Integer>::value>::type>
	explicit static_integer(Integer n):_mp_alloc(0),_mp_size(0),m_limbs()
	{
		const auto orig_n = n;
		limb_t bit_idx = 0;
		while (n != Integer(0)) {
			if (bit_idx == limb_bits * 2u) {
				piranha_throw(std::overflow_error,"insufficient bit width");
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
	template <typename T>
	void fix_sign_ctor(T, typename std::enable_if<std::is_unsigned<T>::value>::type * = nullptr) {}
	template <typename T>
	void fix_sign_ctor(T n, typename std::enable_if<std::is_signed<T>::value>::type * = nullptr)
	{
		if (n < T(0)) {
			negate();
		}
	}
	static_integer(const static_integer &) = default;
	static_integer(static_integer &&) = default;
	~static_integer() noexcept
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
		m_limbs[static_cast<size_type>(quot)] = static_cast<limb_t>(m_limbs[static_cast<size_type>(quot)] | static_cast<limb_t>(limb_t(1) << rem));
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
		return _mp_alloc == 0 && _mp_size <= 2 && _mp_size >= -2 &&
			// Excess bits must be zero for consistency.
			!(static_cast<dlimb_t>(m_limbs[0u]) >> limb_bits) &&
			!(static_cast<dlimb_t>(m_limbs[1u]) >> limb_bits) &&
			(calculate_n_limbs() == _mp_size || -calculate_n_limbs() == _mp_size);
	}
	mpz_size_t abs_size() const
	{
		return (_mp_size >= 0) ? _mp_size : -_mp_size;
	}
	// Convert static integer to a GMP mpz. The out struct must be initialized to zero.
	void to_mpz(mpz_struct_t &out) const
	{
		// mp_bitcnt_t must be able to count all the bits in the static integer.
		static_assert(limb_bits * 2u < boost::integer_traits< ::mp_bitcnt_t>::const_max,"Overflow error.");
		piranha_assert(out._mp_d != nullptr && mpz_cmp_si(&out,0) == 0);
		auto l = m_limbs[0u];
		for (limb_t i = 0u; i < limb_bits; ++i) {
			if (l % 2u) {
				::mpz_setbit(&out,static_cast< ::mp_bitcnt_t>(i));
			}
			l = static_cast<limb_t>(l >> 1u);
		}
		l = m_limbs[1u];
		for (limb_t i = 0u; i < limb_bits; ++i) {
			if (l % 2u) {
				::mpz_setbit(&out,static_cast< ::mp_bitcnt_t>(i + limb_bits));
			}
			l = static_cast<limb_t>(l >> 1u);
		}
		if (_mp_size < 0) {
			// Switch the sign as needed.
			::mpz_neg(&out,&out);
		}
	}
	friend std::ostream &operator<<(std::ostream &os, const static_integer &si)
	{
		mpz_raii m;
		si.to_mpz(m.m_mpz);
		return stream_mpz(os,m.m_mpz);
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
		const auto size0 = _mp_size, size1 = other._mp_size;
		if (size0 < size1) {
			return true;
		} else if (size1 < size0) {
			return false;
		} else {
			const mpz_size_t abs_size = static_cast<mpz_size_t>(size0 >= 0 ? size0 : -size0);
			const int cmp = compare(*this,other,abs_size);
			return (size0 >= 0) ? cmp < 0 : cmp > 0;
		}
	}
	bool operator>(const static_integer &other) const
	{
		const auto size0 = _mp_size, size1 = other._mp_size;
		if (size0 < size1) {
			return false;
		} else if (size1 < size0) {
			return true;
		} else {
			const mpz_size_t abs_size = static_cast<mpz_size_t>(size0 >= 0 ? size0 : -size0);
			const int cmp = compare(*this,other,abs_size);
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
		m_limbs[0u] = static_cast<limb_t>((m_limbs[0u] << delta_bits) >> delta_bits);
		m_limbs[1u] = static_cast<limb_t>((m_limbs[1u] << delta_bits) >> delta_bits);
	}
	template <typename T = static_integer>
	void clear_extra_bits(typename std::enable_if<T::limb_bits == T::total_bits>::type * = nullptr) {}
	static void raw_add(static_integer &res, const static_integer &x, const static_integer &y)
	{
		piranha_assert(x.abs_size() <= 2 && y.abs_size() <= 2);
		const dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(x.m_limbs[0u]) + y.m_limbs[0u]);
		const dlimb_t hi = static_cast<dlimb_t>((static_cast<dlimb_t>(x.m_limbs[1u]) + y.m_limbs[1u]) + (lo >> limb_bits));
		if (unlikely(static_cast<limb_t>(hi >> limb_bits) != 0u)) {
			piranha_throw(std::overflow_error,"overflow in raw addition");
		}
		res.m_limbs[0u] = static_cast<limb_t>(lo);
		res.m_limbs[1u] = static_cast<limb_t>(hi);
		res._mp_size = res.calculate_n_limbs();
		res.clear_extra_bits();
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
	static void add_or_sub(static_integer &res, const static_integer &x, const static_integer &y)
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
			raw_add(res,x,y);
			if (!signx) {
				res.negate();
			}
		} else {
			if (asizex > asizey || (asizex == asizey && compare(x,y,asizex) >= 0)) {
				raw_sub(res,x,y);
				if (!signx) {
					res.negate();
				}
			} else {
				raw_sub(res,y,x);
				if (!signy) {
					res.negate();
				}
			}
		}
	}
	static void add(static_integer &res, const static_integer &x, const static_integer &y)
	{
		add_or_sub<true>(res,x,y);
	}
	static void sub(static_integer &res, const static_integer &x, const static_integer &y)
	{
		add_or_sub<false>(res,x,y);
	}
	static void raw_mul(static_integer &res, const static_integer &x, const static_integer &y, const mpz_size_t &asizex,
		const mpz_size_t &asizey)
	{
		piranha_assert(asizex > 0 && asizey > 0 && asizex <= 1 && asizey <= 1);
		const dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(x.m_limbs[0u]) * y.m_limbs[0u]);
		res.m_limbs[0u] = static_cast<limb_t>(lo);
		const limb_t cy_limb = static_cast<limb_t>(lo >> limb_bits);
		res.m_limbs[1u] = cy_limb;
		res._mp_size = static_cast<mpz_size_t>((asizex + asizey) - mpz_size_t(cy_limb == 0u));
		res.clear_extra_bits();
		piranha_assert(res._mp_size > 0);
	}
	static void mul(static_integer &res, const static_integer &x, const static_integer &y)
	{
		piranha_assert(x.abs_size() <= 1 && y.abs_size() <= 1);
		mpz_size_t asizex = x._mp_size, asizey = y._mp_size;
		if (unlikely(asizex == 0 || asizey == 0)) {
			res._mp_size = 0;
			res.m_limbs[0u] = 0u;
			res.m_limbs[1u] = 0u;
			return;
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
		raw_mul(res,x,y,asizex,asizey);
		if (signx != signy) {
			res.negate();
		}
	}
	static_integer &operator+=(const static_integer &other)
	{
		add(*this,*this,other);
		return *this;
	}
	static_integer &operator-=(const static_integer &other)
	{
		sub(*this,*this,other);
		return *this;
	}
	static_integer &operator*=(const static_integer &other)
	{
		mul(*this,*this,other);
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
	void multiply_accumulate(const static_integer &b, const static_integer &c)
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
		piranha_assert(asizea <= 2 && asizeb <= 1 && asizec <= 1);
		if (unlikely(asizeb == 0 || asizec == 0)) {
			return;
		}
		static_integer tmp;
		raw_mul(tmp,b,c,asizeb,asizec);
		const mpz_size_t asizetmp = tmp._mp_size;
		const bool signtmp = (signb == signc);
		piranha_assert(asizetmp <= 2 && asizetmp > 0);
		if (signa == signtmp) {
			raw_add(*this,*this,tmp);
			if (!signa) {
				negate();
			}
		} else {
			if (asizea > asizetmp || (asizea == asizetmp && compare(*this,tmp,asizea) >= 0)) {
				raw_sub(*this,*this,tmp);
				if (!signa) {
					negate();
				}
			} else {
				raw_sub(*this,tmp,*this);
				if (!signtmp) {
					negate();
				}
			}
		}
	}
	// Left-shift by one.
	void lshift1()
	{
		piranha_assert(m_limbs[1u] < (limb_t(1) << (limb_bits - 1u)));
		using size_type = typename limbs_type::size_type;
		// Shift both limbs.
		dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(m_limbs[0u]) << dlimb_t(1));
		dlimb_t hi = static_cast<dlimb_t>(static_cast<dlimb_t>(m_limbs[1u]) << dlimb_t(1));
		m_limbs[0u] = static_cast<limb_t>(lo);
		m_limbs[1u] = static_cast<limb_t>(hi + (lo >> limb_bits));
		piranha_assert((hi >> limb_bits) != 1u);
		mpz_size_t asize = _mp_size;
		bool sign = true;
		if (asize < 0) {
			asize = -asize;
			sign = false;
		}
		if (asize < 2) {
			asize = static_cast<mpz_size_t>(asize + (m_limbs[static_cast<size_type>(asize)] != 0u));
			_mp_size = static_cast<mpz_size_t>(sign ? asize : -asize);
		}
		clear_extra_bits();
	}
	// Division.
	// http://en.wikipedia.org/wiki/Division_algorithm#Long_division
	static void div(static_integer &q, static_integer &r, const static_integer &a, const static_integer &b)
	{
		piranha_assert(!b.is_zero());
		mpz_size_t asizea = a._mp_size, asizeb = b._mp_size;
		bool signa = true, signb = true;
		if (asizea < 0) {
			asizea = -asizea;
			signa = false;
		}
		if (asizeb < 0) {
			asizeb = -asizeb;
			signb = false;
		}
		// Init q to zero.
		q._mp_size = 0;
		q.m_limbs[0u] = 0u;
		q.m_limbs[1u] = 0u;
		// If |b| > |a|, quotient is 0.
		if (asizeb > asizea || (asizea == asizeb && compare(b,a,asizea) > 0)) {
			// NOTE: remainder will have same sign as a.
			r = a;
			return;
		}
		// Init r to zero.
		r._mp_size = 0;
		r.m_limbs[0u] = 0u;
		r.m_limbs[1u] = 0u;
		const auto a_bits_size = a.bits_size();
		piranha_assert(a_bits_size > 0u);
		for (limb_t i = a_bits_size; i > 0u; --i) {
			const limb_t idx = static_cast<limb_t>(i - 1u);
			r.lshift1();
			if (a.test_bit(idx)) {
				r.set_bit(0u);
			}
			if (r._mp_size > asizeb || (r._mp_size == asizeb && compare(r,b,asizeb) >= 0)) {
				raw_sub(r,r,b);
				q.set_bit(idx);
			}
		}
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
	limb_t bits_size() const
	{
		using size_type = typename limbs_type::size_type;
		const auto asize = abs_size();
		if (asize == 0) {
			return 0u;
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
		return (static_cast<limb_t>(m_limbs[static_cast<size_type>(quot)] & static_cast<limb_t>(limb_t(1u) << rem)) != 0u);
	}
	mpz_alloc_t	_mp_alloc;
	mpz_size_t	_mp_size;
	limbs_type	m_limbs;
};

// Static init.
template <int NBits>
const typename static_integer<NBits>::limb_t static_integer<NBits>::limb_bits;

// Integer union.
template <int NBits>
union integer_union
{
	using s_storage = static_integer<NBits>;
	using d_storage = mpz_struct_t;
	static void move_ctor_mpz(mpz_struct_t &to, mpz_struct_t &from) noexcept
	{
		to._mp_alloc = from._mp_alloc;
		to._mp_size = from._mp_size;
		to._mp_d = from._mp_d;
	}
	// Private implementation of swap.
	static void swap_mpz_t(mpz_struct_t &n1,mpz_struct_t &n2)
	{
		// NOTE: implement swap manually because this function is used in assignment, and hence when potentially
		// reviving moved-from objects. It seems like we do not have the guarantee that ::mpz_swap() is gonna work
		// on uninitialised objects.
		std::swap(n1._mp_d,n2._mp_d);
		std::swap(n1._mp_size,n2._mp_size);
		std::swap(n1._mp_alloc,n2._mp_alloc);
	}
	integer_union():st() {}
	integer_union(const integer_union &other)
	{
		if (other.is_static()) {
			::new (static_cast<void *>(&st)) s_storage(other.st);
		} else {
			::new (static_cast<void *>(&dy)) d_storage;
			::mpz_init_set(&dy,&other.dy);
			piranha_assert(dy._mp_alloc > 0);
		}
	}
	integer_union(integer_union &&other) noexcept
	{
		if (other.is_static()) {
			::new (static_cast<void *>(&st)) s_storage(std::move(other.st));
		} else {
			::new (static_cast<void *>(&dy)) d_storage;
			move_ctor_mpz(dy,other.dy);
			// Downgrade the other to an empty static.
			other.dy.~d_storage();
			::new (static_cast<void *>(&other.st)) s_storage();
		}
	}
	~integer_union() noexcept
	{
		if (is_static()) {
			st.~s_storage();
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
			st = other.st;
		} else if (s1 && !s2) {
			// Destroy static.
			st.~s_storage();
			// Construct the dynamic struct.
			::new (static_cast<void *>(&dy)) d_storage;
			// Init + assign the mpz.
			::mpz_init_set(&dy,&other.dy);
			piranha_assert(dy._mp_alloc > 0);
		} else if (!s1 && s2) {
			// Create a copy of other and promote it.
			auto other_copy(other);
			other_copy.promote();
			::mpz_set(&dy,&other_copy.dy);
		} else {
			::mpz_set(&dy,&other.dy);
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
			st = std::move(other.st);
		} else if (s1 && !s2) {
			// Destroy static.
			st.~s_storage();
			// Construct the dynamic struct.
			::new (static_cast<void *>(&dy)) d_storage;
			move_ctor_mpz(dy,other.dy);
			// Downgrade the other to an empty static.
			other.dy.~d_storage();
			::new (static_cast<void *>(&other.st)) s_storage();
		} else if (!s1 && s2) {
			// Promote directly other, no need for copy.
			other.promote();
			// Swap with the promoted other.
			swap_mpz_t(dy,other.dy);
		} else {
			// Swap with other.
			swap_mpz_t(dy,other.dy);
		}
		return *this;
	}
	bool is_static() const
	{
		return st._mp_alloc == 0;
	}
	void destroy_dynamic()
	{
		piranha_assert(!is_static());
		piranha_assert(dy._mp_alloc > 0);
		piranha_assert(dy._mp_d != nullptr);
		::mpz_clear(&dy);
		dy.~d_storage();
	}
	void promote()
	{
		piranha_assert(is_static());
		mpz_raii tmp;
		st.to_mpz(tmp.m_mpz);
		// Destroy static.
		st.~s_storage();
		// Construct the dynamic struct.
		::new (static_cast<void *>(&dy)) d_storage;
		move_ctor_mpz(dy,tmp.m_mpz);
		tmp.m_mpz._mp_d = nullptr;
	}
	s_storage	st;
	d_storage	dy;
};

}

/// Multi-precision integer class.
/**
 * foo.
 */
template <int NBits>
class mp_integer
{
	public:
		mp_integer() = default;
		mp_integer(const mp_integer &) = default;
		mp_integer(mp_integer &&) = default;
		~mp_integer() = default;
		mp_integer &operator=(const mp_integer &) = default;
		mp_integer &operator=(mp_integer &&) = default;
		friend std::ostream &operator<<(std::ostream &os, const mp_integer &n)
		{
			if (n.m_int.is_static()) {
				return (os << n.m_int.st);
			} else {
				return detail::stream_mpz(os,n.m_int.dy);
			}
		}
		void promote()
		{
			if (unlikely(!m_int.is_static())) {
				piranha_throw(std::invalid_argument,"cannot promote non-static integer");
			}
			m_int.promote();
		}
	private:
		detail::integer_union<NBits> m_int;
};

//using integer = mp_integer<>;

}

#endif
