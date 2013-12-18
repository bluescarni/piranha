#include <algorithm>
#include <boost/integer_traits.hpp>
#include <cmath>
#include <cstddef>
#include <gmp.h>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

using mpz_struct_t = std::remove_extent< ::mpz_t>::type;
using mpz_size_t = decltype(std::declval<mpz_struct_t>()._mp_size);
using mpz_alloc_t = decltype(std::declval<mpz_struct_t>()._mp_alloc);

// NOTE: assumptions to be checked about the mpz_struct_t:
// - the types of the members are implicitly checked by the typedefs above;
// - we need to check in the build system (or maybe via a static assertion somewhere in detail::)
//   that the order of the member is the same and that there's no extra stuff. Maybe do this
//   via pointer comparisons? See also http://en.wikipedia.org/wiki/Offsetof --> note that offsetof works only on
//   standard layout types.
struct static_integer
{
	static const mpz_alloc_t static_size = 3;
	// NOTE: 10 is just a sensible ad-hoc limit - we can always take +/- of 10 regardless of the underlying
	// type, which must have a range of 255 at least.
	static_assert(static_size > 0 && static_size < 10,"Invalid static size.");
	// NOTE: zero is represented in GMP by _mp_size = 0.
	// NOTE: _mp_alloc is used here only as a flag to signal static/dynamic storage.
	static_integer():_mp_alloc(0),_mp_size(0) {}
	static_integer(const static_integer &other):_mp_alloc(0)
	{
		copy(other);
	}
	static_integer(static_integer &&other):_mp_alloc(0)
	{
		copy(other);
	}
	static_integer &operator=(const static_integer &other)
	{
		// TODO likely
		if (this != &other) {
			copy(other);
		}
		return *this;
	}
	static_integer &operator=(static_integer &&other)
	{
		return operator=(other);
	}
	~static_integer() = default;
	void copy(const static_integer &other)
	{
		// TODO assert
		// TODO: need to assert the size in limbs is less than 3 as well!
		_mp_size = other._mp_size;
		for (mpz_alloc_t i = 0; i < static_size; ++i) {
			m_storage[i] = other.m_storage[i];
		}
	}
	// This will return an mpz struct type acting as a proxy for this, meaning
	// that it will have same size as this, alloc of static_size and a pointer to the
	// internal storage of this.
	mpz_struct_t mpz_proxy() const
	{
		return mpz_struct_t{static_size,_mp_size,&m_storage[0]};
	}
	mpz_alloc_t		_mp_alloc;
	mpz_size_t		_mp_size;
	// NOTE: make this mutable as we want to be able to take out a non-const pointer
	// from the const mpz_proxy method above.
	mutable ::mp_limb_t	m_storage[static_size];
};

// TODO:
// - asserts,
// - build-time checks on the mpz type,
// - assert that whenever we init a dynamic integer, the _mp_alloc is not zero - we rely on this behaviour from GMP to
//   distinguish between static and dynamic
// - convert static_asserts in PIRANHA_TT_CHECK.

// NOTE: a lot of uses here of constructors and destructors are redundant, in the sense
// that we are operating on trivial structs (at least regarding the mpz one) and probably there
// is no need to call them explicitly. They are going to be optimised away anyway, we keep them
// for clarity and consistency about union usage. See for instance the comments and some usage
// examples here:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2544.pdf
union integer_union
{
	private:
		using s_storage = static_integer;
		using d_storage = mpz_struct_t;
		// Double check these assumptions in order for the union common initial sequence trick
		// to work.
		// TODO TT_CHECK
		static_assert(std::is_standard_layout<static_integer>::value,"static_integer is not a standard-layout class.");
		static_assert(std::is_standard_layout<mpz_struct_t>::value,"mpz_struct_t is not a standard-layout class.");
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
				::new (static_cast<void *>(&st)) s_storage(std::move(other.st));
			} else {
				// TODO move semantics --> remember to nuke the other.
				::new (static_cast<void *>(&dy)) d_storage;
				::mpz_init_set(&dy,&other.dy);
			}
		}
		integer_union &operator=(const integer_union &other)
		{
			// TODO unlikely
			// TODO make this follow more exception-safe patterns: create new dynamic mpz and then swap it in.
			// TODO assert every time we do an mpz_init that the allocated space is greater than zero.
			if (this == &other) {
				return *this;
			}
			if (is_static()) {
				if (other.is_static()) {
					// st vs st.
					st = other.st;
				} else {
					// st vs dyn.
					const mpz_struct_t tmp_proxy = st.mpz_proxy();
					mpz_struct_t tmp;
					::mpz_init_set(&tmp,&tmp_proxy);
					// Destroy static.
					st.~s_storage();
					// Init the dynamic.
					::new (static_cast<void *>(&dy)) d_storage;
					::mpz_swap(&dy,&tmp);
				}
			} else {
				if (is_static()) {
					// dyn vs st.
					// Destroy the dynamic.
					destroy_dynamic();
					// Init the static.
					::new (static_cast<void *>(&st)) s_storage(other.st);
				} else {
					// dyn vs dyn.
					::mpz_set(&dy,&other.dy);
				}
			}
			return *this;
		}
		integer_union &operator=(integer_union &&other) noexcept
		{
			// TODO move semantics --> nuke other.
			// TODO unlikely
			return operator=(other);
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
		mpz_size_t size() const
		{
			return st._mp_size;
		}
		mpz_size_t abs_size() const
		{
			// TODO assert is_static, as we want to use this only when we know
			// we can safely take abs.
			// TODO static assert on numeric_limits?
			return std::abs(size());
		}
		void upgrade()
		{
			// TODO: assert is_static
			const mpz_struct_t tmp_proxy = st.mpz_proxy();
			mpz_struct_t tmp;
			::mpz_init_set(&tmp,&tmp_proxy);
			// Destroy the static.
			st.~s_storage();
			// Init the dynamic.
			::new (static_cast<void *>(&dy)) d_storage;
			// Swap in the data from tmp.
			::mpz_swap(&dy,&tmp);
			// TODO: assert that _mp_alloc is greater than zero? Or actually that this is not static() any more.
			// Maybe the assertion that mpz_init with zero always allocates something should go into the build system checks
			// for mpz.
		}
		s_storage	st;
		d_storage	dy;
};

class new_integer
{
		static_assert(std::is_standard_layout<integer_union>::value,"integer_union is not a standard-layout class.");
	public:
		new_integer() = default;
		explicit new_integer(int n):m_union()
		{
			// TODO detect if static storage is enough via nbits.
			mpz_struct_t tmp = m_union.st.mpz_proxy();
			::mpz_set_si(&tmp,n);
			m_union.st._mp_size = tmp._mp_size;
		}
		new_integer(const new_integer &) = default;
		new_integer(new_integer &&) = default;
		new_integer &operator=(const new_integer &) = default;
		new_integer &operator=(new_integer &&) = default;
		~new_integer() = default;
		friend std::ostream &operator<<(std::ostream &os, const new_integer &n)
		{
			if (n.m_union.is_static()) {
				const mpz_struct_t tmp = n.m_union.st.mpz_proxy();
				return print_impl(os,&tmp);
			} else {
				return print_impl(os,&n.m_union.dy);
			}
		}
		new_integer &operator+=(const new_integer &other)
		{
			return in_place_add_sub<true,::mpz_add>(other);
		}
		new_integer operator+(const new_integer &other) const
		{
			new_integer retval(*this);
			retval += other;
			return retval;
		}
		new_integer &operator-=(const new_integer &other)
		{
			return in_place_add_sub<false,::mpz_sub>(other);
		}
		new_integer operator-(const new_integer &other) const
		{
			new_integer retval(*this);
			retval -= other;
			return retval;
		}
		new_integer &operator*=(const new_integer &other)
		{
			const unsigned static_score = static_cast<unsigned>(m_union.is_static()) + (static_cast<unsigned>(other.m_union.is_static()) << 1u);
			if (static_score == 3u && m_union.abs_size() + other.m_union.abs_size() <= static_integer::static_size)
			{
				static_mul(*this,*this,other);
			} else {
				in_place_op_dynamic< ::mpz_mul>(static_score,other);
			}
			return *this;
		}
		new_integer &operator*=(int n)
		{
			return operator*=(new_integer(n));
		}
		new_integer operator*(const new_integer &other) const
		{
			new_integer retval(*this);
			retval *= other;
			return retval;
		}
		new_integer &multiply_accumulate(const new_integer &n1, const new_integer &n2)
		{
			const unsigned static_score = static_cast<unsigned>(m_union.is_static()) + (static_cast<unsigned>(n1.m_union.is_static()) << 1u) +
				(static_cast<unsigned>(n2.m_union.is_static()) << 2u);
			if (static_score == 7u && std::max<mpz_size_t>(n1.m_union.abs_size() + n2.m_union.abs_size(),m_union.abs_size()) <
				static_integer::static_size)
			{
				static_mul_add(*this,n1,n2);
			} else {
				switch (static_score) {
					case 0u:
						// ddd.
						::mpz_addmul(&m_union.dy,&n1.m_union.dy,&n2.m_union.dy);
						break;
					case 1u:
					{
						// sdd.
						m_union.upgrade();
						::mpz_addmul(&m_union.dy,&n1.m_union.dy,&n2.m_union.dy);
						break;
					}
					case 2u:
					{
						// dsd.
						auto proxy = n1.m_union.st.mpz_proxy();
						::mpz_addmul(&m_union.dy,&proxy,&n2.m_union.dy);
						break;
					}
					case 3u:
					{
						// ssd.
						m_union.upgrade();
						auto proxy_1 = n1.m_union.st.mpz_proxy();
						::mpz_addmul(&m_union.dy,&proxy_1,&n2.m_union.dy);
						break;
					}
					case 4u:
					{
						// dds.
						auto proxy_2 = n2.m_union.st.mpz_proxy();
						::mpz_addmul(&m_union.dy,&n1.m_union.dy,&proxy_2);
						break;
					}
					case 5u:
					{
						// sds.
						m_union.upgrade();
						auto proxy_2 = n2.m_union.st.mpz_proxy();
						::mpz_addmul(&m_union.dy,&n1.m_union.dy,&proxy_2);
						break;
					}
					case 6u:
					{
						// dss.
						auto proxy_1 = n1.m_union.st.mpz_proxy(), proxy_2 = n2.m_union.st.mpz_proxy();
						::mpz_addmul(&m_union.dy,&proxy_1,&proxy_2);
						break;
					}
					case 7u:
					{
						// sss.
						m_union.upgrade();
						auto proxy_1 = n1.m_union.st.mpz_proxy(), proxy_2 = n2.m_union.st.mpz_proxy();
						::mpz_addmul(&m_union.dy,&proxy_1,&proxy_2);
						break;
					}
				}
			}
			return *this;
		}
		bool operator==(const new_integer &other) const
		{
			const unsigned static_score = static_cast<unsigned>(m_union.is_static()) + (static_cast<unsigned>(other.m_union.is_static()) << 1u);
			// TODO assert on static_score.
			switch (static_score) {
				case 0u:
					// dd.
					return ::mpz_cmp(&m_union.dy,&other.m_union.dy) == 0;
				case 1u:
				{
					// sd.
					const auto proxy = m_union.st.mpz_proxy();
					return ::mpz_cmp(&proxy,&other.m_union.dy) == 0;
				}
				case 2u:
				{
					// ds.
					const auto proxy = other.m_union.st.mpz_proxy();
					return ::mpz_cmp(&m_union.dy,&proxy) == 0;
				}
				case 3u:
				{
					// ss.
					const auto proxy_1 = m_union.st.mpz_proxy(), proxy_2 = other.m_union.st.mpz_proxy();
					return ::mpz_cmp(&proxy_1,&proxy_2) == 0;
				}
			}
		}
		bool operator!=(const new_integer &other) const
		{
			return !operator==(other);
		}
		new_integer operator-() const
		{
			new_integer retval(*this);
			retval.negate();
			return retval;
		}
		bool is_zero() const
		{
			return m_union.size() == 0;
		}
		void negate()
		{
			m_union.st._mp_size = -m_union.st._mp_size;
		}
	private:
		// This function is used to recalculate the actual number of limbs
		// in dst - it will detect most-significant zero limbs and decrease
		// nlimbs accordingly.
		static void mpn_normalise(::mp_limb_t *dst,mpz_size_t &nlimbs)
		{
			while (nlimbs > 0) {
				if (dst[nlimbs - static_cast<mpz_size_t>(1)] != 0) {
					break;
				}
				--nlimbs;
			}
		}
		// Simple helper fuction for copying limbs.
		static void mpn_copy(::mp_limb_t *dst, const ::mp_limb_t *src, mpz_size_t size)
		{
			std::copy(src,src + size,dst);
		}
		static void static_mul_add(new_integer &retval, const new_integer &n1, const new_integer &n2)
		{
			auto proxy_r = retval.m_union.st.mpz_proxy(), proxy_1 = n1.m_union.st.mpz_proxy(),
				proxy_2 = n2.m_union.st.mpz_proxy();
			::mpz_addmul(&proxy_r,&proxy_1,&proxy_2);
			// Update the size in the real result.
			retval.m_union.st._mp_size = proxy_r._mp_size;
			// TODO assert alloc?
		}
		// In-place add/sub/mul when static operations are not possible.
		template <decltype( ::mpz_add) Func>
		void in_place_op_dynamic(unsigned static_score, const new_integer &other)
		{
			switch (static_score) {
				case 0u:
					// Both dynamic.
					Func(&m_union.dy,&m_union.dy,&other.m_union.dy);
					break;
				case 1u:
					// Static vs dynamic.
					m_union.upgrade();
					Func(&m_union.dy,&m_union.dy,&other.m_union.dy);
					break;
				case 2u:
				{
					// Dynamic vs static.
					const auto tmp_proxy = other.m_union.st.mpz_proxy();
					Func(&m_union.dy,&m_union.dy,&tmp_proxy);
					break;
				}
				case 3u:
				{
					// Both are static, but result overflows the static size.
					m_union.upgrade();
					const auto tmp_proxy = other.m_union.st.mpz_proxy();
					Func(&m_union.dy,&m_union.dy,&tmp_proxy);
				}
			}
		}
		template <bool AddOrSub,decltype( ::mpz_add) Func>
		new_integer &in_place_add_sub(const new_integer &other)
		{
			const unsigned static_score = static_cast<unsigned>(m_union.is_static()) + (static_cast<unsigned>(other.m_union.is_static()) << 1u);
			if (static_score == 3u && std::max<mpz_size_t>(m_union.abs_size(),other.m_union.abs_size()) <
				static_integer::static_size)
			{
				// Both operands are static and result will also fit in static storage.
				static_add_sub<true>(*this,*this,other);
			} else {
				in_place_op_dynamic<Func>(static_score,other);
			}
			return *this;
		}
		static void static_mul(new_integer &retval, const new_integer &op1, const new_integer &op2)
		{
			::mp_limb_t *ptr1 = op1.m_union.st.m_storage, *ptr2 = op2.m_union.st.m_storage,
				*res_ptr = retval.m_union.st.m_storage;
			auto size1 = op1.m_union.size(), size2 = op2.m_union.size();
			const bool sign_product = ((size1 >= 0) == (size2 >= 0));
			size1 = std::abs(size1);
			size2 = std::abs(size2);
			::mp_limb_t cy_limb;
			if (size1 < size2) {
				std::swap(ptr1,ptr2);
				std::swap(size1,size2);
			}
			// If the smaller size is zero, result will be zero as well.
			if (size2 == 0) {
				retval.m_union.st._mp_size = 0;
				return;
			}
			if (size2 == 1) {
				// Smaller size is 1, use the special-case mul function.
				// NOTE: no need here to handle overlapping, as mpn_mul_1 will work
				// even for overlapping operands.
				cy_limb = ::mpn_mul_1(res_ptr,ptr1,size1,ptr2[0]);
				// TODO: assert
				res_ptr[size1] = cy_limb;
				size1 += (cy_limb != 0);
				// TODO: assert
				retval.m_union.st._mp_size = (sign_product ? size1 : -size1);
				return;
			}
			// Result size.
			// TODO assert.
			mpz_size_t res_size = size1 + size2;
			// Temporary storage to be used in case of overlapping.
			::mp_limb_t tmp_storage[static_integer::static_size];
			if (res_ptr == ptr1) {
				// Handle overlapping of result and op1.
				ptr1 = tmp_storage;
				if (res_ptr == ptr2) {
					// Handle when op2 also overlaps with result.
					ptr2 = ptr1;
				}
				// NOTE: copy into ptr1, which now is temp storage, op1 - which
				// we know it's the same as res_ptr.
				mpn_copy(ptr1,res_ptr,size1);
			} else if (res_ptr == ptr2) {
				ptr2 = tmp_storage;
				mpn_copy(ptr2,res_ptr,size2);
			}
			if (ptr1 == ptr2) {
				::mpn_sqr(res_ptr,ptr1,size1);
				cy_limb = res_ptr[res_size - 1];
			} else {
				cy_limb = ::mpn_mul(res_ptr,ptr1,size1,ptr2,size2);
			}
			res_size -= (cy_limb == 0);
			retval.m_union.st._mp_size = sign_product ? res_size : -res_size;
		}
		template <bool AddOrSub>
		static void static_add_sub(new_integer &retval, const new_integer &op1, const new_integer &op2)
		{
			auto size1 = op1.m_union.size(), size2 = (AddOrSub) ? op2.m_union.size() : -op2.m_union.size(),
				asize1 = op1.m_union.abs_size(), asize2 = op2.m_union.abs_size();
			const ::mp_limb_t *ptr1 = op1.m_union.st.m_storage, *ptr2 = op2.m_union.st.m_storage;
			::mp_limb_t *res_ptr = retval.m_union.st.m_storage;
			// NOTE: GMP functions in general expect the first argument to be not smaller (in limbs) than the second.
			if (asize1 < asize2) {
				std::swap(ptr1,ptr2);
				std::swap(size1,size2);
				std::swap(asize1,asize2);
			}
			// This will be the final size.
			mpz_size_t res_size;
			const bool sign1 = (size1 >= 0), sign2 = (size2 >= 0);
			if (sign1 == sign2) {
				// Same sign, treat as a normal addition and change sign in the end if needed.
				// This is the carry limb, will be zero if there is no carry, 1 otherwise.
				const auto cy_limb = ::mpn_add(res_ptr,ptr1,asize1,ptr2,asize2);
				// TODO assert not out of bounds.
				res_ptr[asize1] = cy_limb;
				res_size = asize1 + static_cast<mpz_size_t>(cy_limb);
				if (!sign1) {
					res_size = -res_size;
				}
			} else {
				// Treat as subtraction.
				if (asize1 != asize2) {
					// NOTE: here we know asize1 > asize2 --> TODO assert
					// NOTE: no need for carry limb, subtraction will never result in increase in size.
					::mpn_sub(res_ptr,ptr1,asize1,ptr2,asize2);
					res_size = asize1;
					mpn_normalise(res_ptr,res_size);
					if (!sign1) {
						res_size = -res_size;
					}
				} else if (::mpn_cmp(ptr1,ptr2,asize1) < 0) {
					::mpn_sub_n(res_ptr,ptr2,ptr1,asize1);
					res_size = asize1;
					mpn_normalise(res_ptr,res_size);
					if (sign1) {
						res_size = -res_size;
					}
				} else {
					::mpn_sub_n(res_ptr,ptr1,ptr2,asize1);
					res_size = asize1;
					mpn_normalise(res_ptr,res_size);
					if (!sign1) {
						res_size = -res_size;
					}
				}
			}
			// Assign the result's size.
			retval.m_union.st._mp_size = res_size;
			// TODO assert not too big just for sure.
		}
		static std::ostream &print_impl(std::ostream &os, const mpz_struct_t *ptr)
		{
			const std::size_t size_base10 = ::mpz_sizeinbase(ptr,10);
			// TODO unlikely
			if (size_base10 > boost::integer_traits<std::size_t>::const_max - static_cast<std::size_t>(2)) {
				// TODO throw
				//piranha_throw(std::overflow_error,"number of digits is too large");
			}
			const auto total_size = size_base10 + 2u;
			// TODO: use small_vector here.
			std::vector<char> tmp(static_cast<std::vector<char>::size_type>(total_size));
			// TODO unlikely
			if (tmp.size() != total_size) {
				// TODO throw error
				//piranha_throw(std::overflow_error,"number of digits is too large");
			}
			os << ::mpz_get_str(&tmp[0u],10,ptr);
			return os;
		}
	private:
		integer_union m_union;
};
