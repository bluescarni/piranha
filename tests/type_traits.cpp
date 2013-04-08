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

#include "../src/type_traits.hpp"

#define BOOST_TEST_MODULE type_traits_test
#include <boost/test/unit_test.hpp>

#include <complex>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/monomial.hpp"

using namespace piranha;

PIRANHA_DECLARE_HAS_TYPEDEF(foo_type);

struct foo
{
	typedef int foo_type;
};

struct bar {};

BOOST_AUTO_TEST_CASE(type_traits_has_typedef_test)
{
	environment env;
	BOOST_CHECK(has_typedef_foo_type<foo>::value);
	BOOST_CHECK(!has_typedef_foo_type<bar>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_is_cv_ref)
{
	BOOST_CHECK_EQUAL(is_cv_or_ref<int>::value,false);
	BOOST_CHECK_EQUAL(is_cv_or_ref<int &>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<const int>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<const volatile int>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<const volatile int &>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<volatile int>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<int * const>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<int const *>::value,false);
}

BOOST_AUTO_TEST_CASE(type_traits_is_nonconst_rvalue_ref_test)
{
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<int>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<int &>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const int>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const volatile int>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const volatile int &>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<volatile int>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<volatile int &&>::value,true);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<volatile int const &&>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const int &&>::value,false);
	BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<int &&>::value,true);
}

struct trivial {};

struct nontrivial_copy
{
	nontrivial_copy(nontrivial_copy &&) noexcept(false) {}
	nontrivial_copy &operator=(nontrivial_copy &&) noexcept(false)
	{
		return *this;
	}
	nontrivial_copy(const nontrivial_copy &other):n(other.n) {}
	int n;
};

struct nontrivial_dtor
{
	nontrivial_dtor(const nontrivial_dtor &) = default;
	nontrivial_dtor(nontrivial_dtor &&) noexcept(false) {}
	nontrivial_dtor &operator=(nontrivial_dtor &&) noexcept(false)
	{
		return *this;
	}
	~nontrivial_dtor() noexcept(false)
	{
		n = 0;
	}
	int n;
};

BOOST_AUTO_TEST_CASE(type_traits_is_trivially_copyable)
{
	BOOST_CHECK_EQUAL(is_trivially_copyable<int>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_copyable<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_copyable<nontrivial_dtor>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_copyable<nontrivial_copy>::value,false);
	BOOST_CHECK_EQUAL(is_trivially_copyable<std::string>::value,false);
}

BOOST_AUTO_TEST_CASE(type_traits_is_trivially_destructible)
{
	BOOST_CHECK_EQUAL(is_trivially_destructible<int>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_destructible<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_destructible<nontrivial_copy>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_destructible<nontrivial_dtor>::value,false);
	BOOST_CHECK_EQUAL(is_trivially_destructible<std::string>::value,false);
}

BOOST_AUTO_TEST_CASE(type_traits_nothrow_type_traits)
{
	BOOST_CHECK_EQUAL(is_nothrow_move_constructible<int>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_move_constructible<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_move_constructible<integer>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_move_constructible<nontrivial_dtor>::value,false);
	BOOST_CHECK_EQUAL(is_nothrow_move_constructible<nontrivial_copy>::value,false);
	BOOST_CHECK_EQUAL(is_nothrow_move_assignable<int>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_move_assignable<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_move_assignable<integer>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_move_assignable<nontrivial_dtor>::value,false);
	BOOST_CHECK_EQUAL(is_nothrow_move_assignable<nontrivial_copy>::value,false);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<integer>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_dtor>::value,false);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_copy>::value,true);
}

BOOST_AUTO_TEST_CASE(type_traits_is_tuple)
{
	BOOST_CHECK(is_tuple<std::tuple<>>::value);
	BOOST_CHECK(is_tuple<std::tuple<int>>::value);
	BOOST_CHECK(!is_tuple<std::string>::value);
}

namespace piranha
{

template <typename T>
class has_degree<T,typename std::enable_if<std::is_same<trivial,T>::value>::type>
{
	public:
		static const bool value = true;
		int get(const T &, const std::set<std::string> & = std::set<std::string>{});
		int lget(const T &, const std::set<std::string> & = std::set<std::string>{});
};

}

BOOST_AUTO_TEST_CASE(type_traits_has_degree)
{
	BOOST_CHECK(!has_degree<int>::value);
	BOOST_CHECK(!has_degree<double>::value);
	BOOST_CHECK(!has_degree<integer>::value);
	BOOST_CHECK(has_degree<trivial>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_is_addable)
{
	BOOST_CHECK((is_addable<int,int>::value));
	BOOST_CHECK((is_addable<int,double>::value));
	BOOST_CHECK((is_addable<double,int>::value));
	BOOST_CHECK((is_addable<std::complex<double>,double>::value));
	BOOST_CHECK((is_addable<double,std::complex<double>>::value));
	BOOST_CHECK((is_addable<double,integer>::value));
	BOOST_CHECK((is_addable<integer,double>::value));
	BOOST_CHECK((!is_addable<trivial,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<int,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<integer,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<std::complex<double>,int>::value));
	BOOST_CHECK((!is_addable<std::complex<double>,integer>::value));
	BOOST_CHECK((is_addable<std::string,std::string>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_addable_in_place)
{
	BOOST_CHECK((is_addable_in_place<int,int>::value));
	BOOST_CHECK((is_addable_in_place<int,double>::value));
	BOOST_CHECK((is_addable_in_place<double,int>::value));
	BOOST_CHECK((is_addable_in_place<std::complex<double>,double>::value));
	BOOST_CHECK((!is_addable_in_place<double,std::complex<double>>::value));
	BOOST_CHECK((!is_addable_in_place<trivial,std::complex<double>>::value));
	BOOST_CHECK((is_addable_in_place<std::string,std::string>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_subtractable)
{
	BOOST_CHECK((is_subtractable<int,int>::value));
	BOOST_CHECK((is_subtractable<int,double>::value));
	BOOST_CHECK((is_subtractable<double,int>::value));
	BOOST_CHECK((is_subtractable<std::complex<double>,double>::value));
	BOOST_CHECK((is_subtractable<double,std::complex<double>>::value));
	BOOST_CHECK((is_subtractable<double,integer>::value));
	BOOST_CHECK((is_subtractable<integer,double>::value));
	BOOST_CHECK((!is_subtractable<trivial,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<int,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<integer,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<std::complex<double>,int>::value));
	BOOST_CHECK((!is_subtractable<std::complex<double>,integer>::value));
	BOOST_CHECK((!is_subtractable<std::string,std::string>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_subtractable_in_place)
{
	BOOST_CHECK((is_subtractable_in_place<int,int>::value));
	BOOST_CHECK((is_subtractable_in_place<int,double>::value));
	BOOST_CHECK((is_subtractable_in_place<double,int>::value));
	BOOST_CHECK((is_subtractable_in_place<std::complex<double>,double>::value));
	BOOST_CHECK((!is_subtractable_in_place<double,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable_in_place<trivial,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable_in_place<std::string,std::string>::value));
}

struct frob
{
	bool operator==(const frob &) const;
	bool operator!=(const frob &) const;
};

struct frob_nonconst
{
	bool operator==(const frob_nonconst &);
	bool operator!=(const frob_nonconst &);
};

struct frob_nonbool
{
	char operator==(const frob_nonbool &) const;
	char operator!=(const frob_nonbool &) const;
};

struct frob_void
{
	void operator==(const frob_nonbool &) const;
	void operator!=(const frob_nonbool &) const;
};

struct frob_copy
{
	int operator==(frob_copy) const;
	int operator!=(frob_copy) const;
};

struct frob_mix
{};

short operator==(const frob_mix &, frob_mix);
short operator!=(const frob_mix &, frob_mix);

struct frob_mix_wrong
{};

short operator==(frob_mix_wrong, frob_mix_wrong &);
short operator!=(frob_mix_wrong, frob_mix_wrong &);

struct frob_mix_not_ineq
{};

bool operator==(const frob_mix_not_ineq &, const frob_mix_not_ineq &);

struct frob_mix_not_eq
{};

bool operator!=(const frob_mix_not_eq &, const frob_mix_not_eq &);

BOOST_AUTO_TEST_CASE(type_traits_is_equality_comparable)
{
	BOOST_CHECK(is_equality_comparable<int>::value);
	BOOST_CHECK(!is_equality_comparable<trivial>::value);
	BOOST_CHECK((is_equality_comparable<int,double>::value));
	BOOST_CHECK((is_equality_comparable<double,int>::value));
	BOOST_CHECK((!is_equality_comparable<double,trivial>::value));
	BOOST_CHECK((!is_equality_comparable<trivial,double>::value));
	BOOST_CHECK(is_equality_comparable<int &>::value);
	BOOST_CHECK(is_equality_comparable<int *>::value);
	BOOST_CHECK((is_equality_comparable<int const *, int *>::value));
	BOOST_CHECK((is_equality_comparable<int &, double>::value));
	BOOST_CHECK((is_equality_comparable<int const &, double &&>::value));
	BOOST_CHECK(is_equality_comparable<frob>::value);
	BOOST_CHECK(!is_equality_comparable<frob_nonconst>::value);
	BOOST_CHECK(is_equality_comparable<frob_nonbool>::value);
	BOOST_CHECK(!is_equality_comparable<frob_void>::value);
	BOOST_CHECK(is_equality_comparable<frob_copy>::value);
	BOOST_CHECK(is_equality_comparable<frob_mix>::value);
	BOOST_CHECK(!is_equality_comparable<frob_mix_wrong>::value);
	BOOST_CHECK(!is_equality_comparable<frob_mix_not_ineq>::value);
	BOOST_CHECK(!is_equality_comparable<frob_mix_not_eq>::value);
}

struct c_element {};

struct nc_element1
{
	nc_element1() = delete;
};

struct nc_element2
{
	nc_element2() = default;
	nc_element2(const nc_element2 &) = default;
	nc_element2(nc_element2 &&) = default;
	nc_element2 &operator=(nc_element2 &&);
};

struct c_element2
{
	c_element2() = default;
	c_element2(const c_element2 &) = default;
	c_element2(c_element2 &&) = default;
	c_element2 &operator=(c_element2 &&) noexcept(true);
};

BOOST_AUTO_TEST_CASE(type_traits_is_container_element)
{
	BOOST_CHECK(is_container_element<int>::value);
	BOOST_CHECK(is_container_element<double>::value);
	BOOST_CHECK(is_container_element<integer>::value);
	BOOST_CHECK(is_container_element<monomial<int>>::value);
	BOOST_CHECK(is_container_element<c_element>::value);
	BOOST_CHECK(!is_container_element<nc_element1>::value);
	BOOST_CHECK(!is_container_element<nc_element2>::value);
	BOOST_CHECK(is_container_element<c_element2>::value);
}
