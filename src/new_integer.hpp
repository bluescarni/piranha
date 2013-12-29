#include <algorithm>
#include <cstdint>
#include <gmp.h>
#include <type_traits>
#include <utility>

#include "config.hpp"

namespace piranha { namespace detail {

// mpz_t is an array of some struct.
using mpz_struct_t = std::remove_extent< ::mpz_t>::type;
// Integral types used for allocation size and number of limbs.
using mpz_alloc_t = decltype(std::declval<mpz_struct_t>()._mp_alloc);
using mpz_size_t = decltype(std::declval<mpz_struct_t>()._mp_size);

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
	using limb_t = std::uint_fast64_t;
	using dlimb_t = PIRANHA_UINT128_T;
	static const limb_t limb_bits = 64;
};
#endif

template <>
struct si_limb_types<32>
{
	using limb_t = std::uint_fast32_t;
	using dlimb_t = std::uint_fast64_t;
	static const limb_t limb_bits = 32;
};

template <>
struct si_limb_types<16>
{
	using limb_t = std::uint_fast16_t;
	using dlimb_t = std::uint_fast32_t;
	static const limb_t limb_bits = 16;
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

template <int NBits>
struct static_integer
{
	using dlimb_t = typename si_limb_types<NBits>::dlimb_t;
	using limb_t = typename si_limb_types<NBits>::limb_t;
	static const limb_t limb_bits = si_limb_types<NBits>::limb_bits;
	// NOTE: better to init everything otherwise zero is gonna be represented by undefined
	// values in lo/mid/hi.
	static_integer():_mp_alloc(0),_mp_size(0),m_lo(0),m_mid(0),m_hi(0) {}
	static_integer(const static_integer &) = default;
	static_integer(static_integer &&) = default;
	~static_integer() noexcept
	{
		// TODO assert sizes.
		// TODO assert consistency checks as well.
	}
	static_integer &operator=(const static_integer &) = default;
	static_integer &operator=(static_integer &&) = default;
	// TODO test 0 == 0.
	/*bool operator==(const static_integer &other) const
	{
		// TODO assert alloc.
		return _mp_size == other._mp_size && m_lo == other.m_lo && m_mid == other.m_mid && m_hi == other.m_hi;
	}
	bool operator!=(const static_integer &other) const
	{
		return !operator==(other);
	}
	bool is_zero() const
	{
		return _mp_size == 0;
	}
	static_integer &operator+=(const static_integer &other)
	{
		add(*this,*this,other);
		return *this;
	}
	static_integer operator+(const static_integer &other) const
	{
		static_integer retval(*this);
		retval += other;
		return retval;
	}
	static_integer &operator-=(const static_integer &other)
	{
		add(*this,*this,other);
		return *this;
	}
	static_integer operator-(const static_integer &other) const
	{
		static_integer retval(*this);
		retval += other;
		return retval;
	}
	static_integer &operator*=(const static_integer &other)
	{
		mult(*this,*this,other);
		return *this;
	}
	static_integer operator*(const static_integer &other) const
	{
		static_integer retval(*this);
		retval *= other;
		return retval;
	}
	static_integer &multiply_accumulate(const static_integer &n1, const static_integer &n2)
	{
		static_integer tmp;
		mult(tmp,n1,n2);
		add(*this,*this,tmp);
		return *this;
	}
	friend std::ostream &operator<<(std::ostream &os, const static_integer &)
	{
		return os;
	}
	static void add(static_integer &res, const static_integer &x, const static_integer &y)
	{
		// TODO: assert sizes here -> max 2.
		const dlimb_t lo = static_cast<dlimb_t>(x.m_lo) + y.m_lo;
		const dlimb_t mid = static_cast<dlimb_t>(x.m_mid) + y.m_mid + (lo >> limb_bits);
		res.m_lo = static_cast<limb_t>(lo);
		res.m_mid = static_cast<limb_t>(mid);
		res.m_hi = static_cast<limb_t>(mid >> limb_bits);
		// TODO: abs sizes here.
		res._mp_size = std::max(x._mp_size,y._mp_size) + (res.m_hi != 0);
	}
	static void mult(static_integer &res, const static_integer &x, const static_integer &y)
	{
		// TODO: assert sizes here -> max 1.
		const dlimb_t lo = static_cast<dlimb_t>(x.m_lo) * y.m_lo;
		res.m_lo = static_cast<limb_t>(lo);
		res.m_mid = static_cast<limb_t>(lo >> limb_bits);
		res.m_hi = 0;
		res._mp_size = 2;
	}*/
	mpz_alloc_t	_mp_alloc;
	mpz_size_t	_mp_size;
	limb_t		m_lo;
	limb_t		m_mid;
	limb_t		m_hi;
};

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
