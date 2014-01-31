#include <algorithm>
#include <array>
#include <boost/integer_traits.hpp>
#include <cstddef>
#include <cstdint>
#include <gmp.h>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "exceptions.hpp"
#include "small_vector.hpp"

namespace piranha { namespace detail {

// mpz_t is an array of some struct.
using mpz_struct_t = std::remove_extent< ::mpz_t>::type;
// Integral types used for allocation size and number of limbs.
using mpz_alloc_t = decltype(std::declval<mpz_struct_t>()._mp_alloc);
using mpz_size_t = decltype(std::declval<mpz_struct_t>()._mp_size);

// Some misc checks to check that the mpz struct conforms to our expectations.
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
	}
	mpz_raii(const mpz_raii &) = delete;
	mpz_raii(mpz_raii &&) = delete;
	mpz_raii &operator=(const mpz_raii &) = delete;
	mpz_raii &operator=(mpz_raii &&) = delete;
	~mpz_raii() noexcept
	{
		::mpz_clear(&m_mpz);
	}
	mpz_struct_t m_mpz;
};

template <int NBits>
struct static_integer
{
	using dlimb_t = typename si_limb_types<NBits>::dlimb_t;
	using limb_t = typename si_limb_types<NBits>::limb_t;
	static const limb_t limb_bits = si_limb_types<NBits>::limb_bits;
	using limbs_type = std::array<limb_t,std::size_t(3)>;
	// Check: we need to be able to address all bits in the 3 limbs using limb_t.
	static_assert(limb_bits < boost::integer_traits<limb_t>::const_max / 3u,"Overflow error.");
	// NOTE: init everything otherwise zero is gonna be represented by undefined
	// values in lo/mid/hi.
	static_integer():_mp_alloc(0),_mp_size(0),m_limbs() {}
	template <typename Integer, typename = typename std::enable_if<std::is_integral<Integer>::value>::type>
	explicit static_integer(Integer n):_mp_alloc(0),_mp_size(0),m_limbs()
	{
		const auto orig_n = n;
		limb_t bit_idx = 0;
		while (n != Integer(0)) {
			if (bit_idx == limb_bits * 3u) {
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
		// NOTE: this is 3 at most, no danger in taking the negative.
		_mp_size = -_mp_size;
	}
	void set_bit(const limb_t &idx)
	{
		using size_type = typename limbs_type::size_type;
		piranha_assert(idx < limb_bits * 3u);
		// Crossing fingers for compiler optimising this out.
		const auto quot = idx / limb_bits, rem = idx % limb_bits;
		m_limbs[static_cast<size_type>(quot)] = static_cast<limb_t>(m_limbs[static_cast<size_type>(quot)] | limb_t(1) << rem);
		// Update the size if needed.
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
		if (m_limbs[2u] != 0) {
			return 3;
		}
		if (m_limbs[1u] != 0) {
			return 2;
		}
		if (m_limbs[0u] != 0) {
			return 1;
		}
		return 0;
	}
	bool consistency_checks() const
	{
		return _mp_alloc == 0 && _mp_size <= 3 && _mp_size >= -3 && (calculate_n_limbs() == _mp_size || -calculate_n_limbs() == _mp_size);
	}
	mpz_size_t abs_size() const
	{
		return (_mp_size >= 0) ? _mp_size : -_mp_size;
	}
	// Convert static integer to a GMP mpz. The out struct must be initialized to zero.
	void to_mpz(mpz_struct_t &out) const
	{
		// mp_bitcnt_t must be able to count all the bits in the static integer.
		static_assert(limb_bits * 3u < boost::integer_traits< ::mp_bitcnt_t>::const_max,"Overflow error.");
		piranha_assert(out._mp_d != nullptr && mpz_cmp_si(&out,0) == 0);
		auto l = m_limbs[0u];
		for (limb_t i = 0u; i < limb_bits; ++i) {
			const auto bit = l % 2u;
			if (bit) {
				::mpz_setbit(&out,static_cast< ::mp_bitcnt_t>(i));
			}
			l = static_cast<limb_t>(l >> 1u);
		}
		l = m_limbs[1u];
		for (limb_t i = 0u; i < limb_bits; ++i) {
			const auto bit = l % 2u;
			if (bit) {
				::mpz_setbit(&out,static_cast< ::mp_bitcnt_t>(i + limb_bits));
			}
			l = static_cast<limb_t>(l >> 1u);
		}
		l = m_limbs[2u];
		for (limb_t i = 0u; i < limb_bits; ++i) {
			const auto bit = l % 2u;
			if (bit) {
				::mpz_setbit(&out,static_cast< ::mp_bitcnt_t>(i + limb_bits * 2u));
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
		const std::size_t size_base10 = ::mpz_sizeinbase(&m.m_mpz,10);
		if (unlikely(size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2))) {
			piranha_throw(std::overflow_error,"number of digits is too large");
		}
		const auto total_size = size_base10 + 2u;
		small_vector<char> tmp;
		tmp.resize(static_cast<small_vector<char>::size_type>(total_size));
		if (unlikely(tmp.size() != total_size)) {
			piranha_throw(std::overflow_error,"number of digits is too large");
		}
		os << ::mpz_get_str(&tmp[0u],10,&m.m_mpz);
		return os;
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
		piranha_assert(size >= 0 && size <= 3);
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
	static void raw_add(static_integer &res, const static_integer &x, const static_integer &y)
	{
		piranha_assert(x.abs_size() <= 2 && y.abs_size() <= 2);
		const dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(x.m_limbs[0u]) + y.m_limbs[0u]);
		const dlimb_t mid = static_cast<dlimb_t>((static_cast<dlimb_t>(x.m_limbs[1u]) + y.m_limbs[1u]) + (lo >> limb_bits));
		res.m_limbs[0u] = static_cast<limb_t>(lo);
		res.m_limbs[1u] = static_cast<limb_t>(mid);
		res.m_limbs[2u] = static_cast<limb_t>(mid >> limb_bits);
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
		res.m_limbs[2u] = limb_t(0u);
		if (res.m_limbs[1u] != 0u) {
			res._mp_size = 2;
		} else if (res.m_limbs[0u] != 0u) {
			res._mp_size = 1;
		} else {
			res._mp_size = 0;
		}
	}
	template <bool AddOrSub>
	static void add_or_sub(static_integer &res, const static_integer &x, const static_integer &y)
	{
		using size_type = typename limbs_type::size_type;
		mpz_size_t asizex = x._mp_size, asizey = AddOrSub ? y._mp_size : -y._mp_size;
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
			const mpz_size_t max_asize = std::max(asizex,asizey);
			piranha_assert(res.m_limbs[static_cast<size_type>(max_asize)] == 0u || res.m_limbs[static_cast<size_type>(max_asize)] == 1u);
			res._mp_size = static_cast<mpz_size_t>(max_asize + mpz_size_t(res.m_limbs[static_cast<size_type>(max_asize)]));
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
		res.m_limbs[2u] = 0u;
		res._mp_size = static_cast<mpz_size_t>((asizex + asizey) - mpz_size_t(cy_limb == 0u));
		piranha_assert(res._mp_size > 0);
	}
	static void mul(static_integer &res, const static_integer &x, const static_integer &y)
	{
		piranha_assert(x.abs_size() <= 1 && y.abs_size() <= 1);
		mpz_size_t asizex = x._mp_size, asizey = y._mp_size;
		if (asizex == 0 || asizey == 0) {
			res._mp_size = 0;
			res.m_limbs[0u] = 0u;
			res.m_limbs[1u] = 0u;
			res.m_limbs[2u] = 0u;
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
		using size_type = typename limbs_type::size_type;
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
			const mpz_size_t max_asize = std::max(asizea,asizetmp);
			piranha_assert(m_limbs[static_cast<size_type>(max_asize)] == 0u || m_limbs[static_cast<size_type>(max_asize)] == 1u);
			_mp_size = static_cast<mpz_size_t>(max_asize + mpz_size_t(m_limbs[static_cast<size_type>(max_asize)]));
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
		piranha_assert(m_limbs[2u] < (limb_t(1) << (limb_bits - 1u)));
		using size_type = typename limbs_type::size_type;
		dlimb_t lo = static_cast<dlimb_t>(static_cast<dlimb_t>(m_limbs[0u]) << dlimb_t(1));
		dlimb_t mid = static_cast<dlimb_t>(static_cast<dlimb_t>(m_limbs[1u]) << dlimb_t(1));
		dlimb_t hi = static_cast<dlimb_t>(static_cast<dlimb_t>(m_limbs[2u]) << dlimb_t(1));
		m_limbs[0u] = static_cast<limb_t>(lo);
		m_limbs[1u] = static_cast<limb_t>(mid + (lo >> limb_bits));
		m_limbs[2u] = static_cast<limb_t>(hi + (mid >> limb_bits));
		piranha_assert((hi >> limb_bits) != 1u);
		mpz_size_t asize = _mp_size;
		bool sign = true;
		if (asize < 0) {
			asize = -asize;
			sign = false;
		}
		if (asize < 3) {
			asize += m_limbs[static_cast<size_type>(asize)] != 0u;
			_mp_size = sign ? asize : -asize;
		}
	}
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
		q.m_limbs[2u] = 0u;
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
		r.m_limbs[2u] = 0u;
	}
	limb_t bits_size() const
	{
		using size_type = typename limbs_type::size_type;
		const auto asize = abs_size();
		if (asize == 0) {
			return 0u;
		}
		const auto idx = static_cast<size_type>(asize - 1);
		limb_t size = static_cast<limb_t>(limb_bits * idx), limb = limbs[idx];
		while (limb != 0u) {
			++size;
			limb >>= 1u;
		}
		return size;
	}
	mpz_alloc_t	_mp_alloc;
	mpz_size_t	_mp_size;
	limbs_type	m_limbs;
};

// Static init.
template <int NBits>
const typename static_integer<NBits>::limb_t static_integer<NBits>::limb_bits;

template <int NBits>
union integer_union
{
	private:
		using s_storage = static_integer<NBits>;
		using d_storage = mpz_struct_t;
	public:
		integer_union():st() {}
		integer_union(const integer_union &other)
		{
			if (other.is_static()) {
				::new (static_cast<void *>(&st)) s_storage(other.st);
			} else {
				::new (static_cast<void *>(&dy)) d_storage;
				::mpz_init_set(&dy,&other.dy);
			}
		}
		integer_union(integer_union &&other) noexcept
		{
			if (other.is_static()) {
				::new (static_cast<void *>(&st)) s_storage(other.st);
			} else {
				// TODO move semantics.
				::new (static_cast<void *>(&dy)) d_storage;
				::mpz_init_set(&dy,&other.dy);
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
		bool is_static() const
		{
			return st._mp_alloc == 0;
		}
		void upgrade()
		{
			mpz_struct_t new_mpz;
			::mpz_init(&new_mpz);
			auto lo = st.m_lo;
			
		}
	private:
		void destroy_dynamic()
		{
			// TODO assert(!is_static())
			// Handle moved-from objects.
			if (dy._mp_d != nullptr) {
				// TODO: assert assert
				::mpz_clear(&dy);
			}
			dy.~d_storage();
		}
	private:
		s_storage	st;
		d_storage	dy;
};

}}
