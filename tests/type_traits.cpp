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

#include <boost/integer_traits.hpp>
#include <complex>
#include <cstddef>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../src/base_term.hpp"
#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/monomial.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

PIRANHA_DECLARE_HAS_TYPEDEF(foo_type);

struct foo
{
	typedef int foo_type;
};

struct bar {};

struct frobniz
{
	typedef int & foo_type;
};

struct frobniz2
{
	typedef int const & foo_type;
};

BOOST_AUTO_TEST_CASE(type_traits_has_typedef_test)
{
	environment env;
	BOOST_CHECK(has_typedef_foo_type<foo>::value);
	BOOST_CHECK(!has_typedef_foo_type<bar>::value);
	BOOST_CHECK(has_typedef_foo_type<frobniz>::value);
	BOOST_CHECK(has_typedef_foo_type<frobniz2>::value);
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

struct trivial_copy
{
	trivial_copy(trivial_copy &&) = default;
	trivial_copy(const trivial_copy &) = default;
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

BOOST_AUTO_TEST_CASE(type_traits_is_nothrow_destructible_test)
{
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<const int>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<const trivial>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_dtor>::value,false);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_copy>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int *>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int &>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int &&>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int const *>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<int const &>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_dtor &>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_copy &>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_dtor *>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<nontrivial_copy *>::value,true);
}

BOOST_AUTO_TEST_CASE(type_traits_is_addable_test)
{
	BOOST_CHECK(is_addable<int>::value);
	BOOST_CHECK(is_addable<const int>::value);
	BOOST_CHECK((is_addable<const int, int>::value));
	BOOST_CHECK((is_addable<int, const int>::value));
	BOOST_CHECK((is_addable<const int &, int &>::value));
	BOOST_CHECK((is_addable<int &&, const int &>::value));
	BOOST_CHECK(is_addable<double>::value);
	BOOST_CHECK(is_addable<std::complex<double>>::value);
	BOOST_CHECK((is_addable<const std::complex<double>,double>::value));
	BOOST_CHECK((is_addable<std::complex<double>,const double>::value));
	BOOST_CHECK((is_addable<int,int>::value));
	BOOST_CHECK((is_addable<int,double>::value));
	BOOST_CHECK((is_addable<double,int>::value));
	BOOST_CHECK((is_addable<std::complex<double>,double>::value));
	BOOST_CHECK((is_addable<double,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<trivial,std::complex<double>>::value));
// The Intel compiler seems to have some nonstandard extensions to the complex class.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK((!is_addable<int,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<std::complex<double>,int>::value));
#endif
	BOOST_CHECK((is_addable<std::string,std::string>::value));
	BOOST_CHECK((is_addable<std::string,const char *>::value));
	BOOST_CHECK((is_addable<const char *,std::string>::value));
	BOOST_CHECK((is_addable<int *,std::size_t>::value));
	BOOST_CHECK((is_addable<std::size_t,int *>::value));
	BOOST_CHECK(!is_addable<int *>::value);
	BOOST_CHECK(is_addable<int &>::value);
	BOOST_CHECK((is_addable<int &, double &>::value));
	BOOST_CHECK((is_addable<double &, int &>::value));
	BOOST_CHECK(is_addable<int const &>::value);
	BOOST_CHECK((is_addable<int const &, double &>::value));
	BOOST_CHECK((is_addable<double const &, int &>::value));
	BOOST_CHECK(is_addable<int &&>::value);
	BOOST_CHECK((is_addable<int &&, double &&>::value));
	BOOST_CHECK((is_addable<double &&, int &&>::value));
	BOOST_CHECK((!is_addable<int &&, std::string &>::value));
	BOOST_CHECK((is_addable<int * &,int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_addable_in_place_test)
{
	BOOST_CHECK((is_addable_in_place<int>::value));
	BOOST_CHECK((is_addable_in_place<int,int>::value));
	BOOST_CHECK((is_addable_in_place<int,double>::value));
	BOOST_CHECK((is_addable_in_place<double,int>::value));
	BOOST_CHECK((is_addable_in_place<std::complex<double>,double>::value));
	BOOST_CHECK((!is_addable_in_place<double,std::complex<double>>::value));
	BOOST_CHECK((!is_addable_in_place<trivial,std::complex<double>>::value));
	BOOST_CHECK((is_addable_in_place<std::string,std::string>::value));
	BOOST_CHECK((is_addable_in_place<int, const int>::value));
	BOOST_CHECK((!is_addable_in_place<const int, int>::value));
	BOOST_CHECK((!is_addable_in_place<const int &, int>::value));
	BOOST_CHECK((is_addable_in_place<int &&, const int &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_subtractable_test)
{
	BOOST_CHECK(is_subtractable<int>::value);
	BOOST_CHECK(is_subtractable<const int>::value);
	BOOST_CHECK((is_subtractable<const int, int>::value));
	BOOST_CHECK((is_subtractable<int, const int>::value));
	BOOST_CHECK((is_subtractable<const int &, int &>::value));
	BOOST_CHECK((is_subtractable<int &&, const int &>::value));
	BOOST_CHECK(is_subtractable<double>::value);
	BOOST_CHECK(is_subtractable<std::complex<double>>::value);
	BOOST_CHECK((is_subtractable<const std::complex<double>,double>::value));
	BOOST_CHECK((is_subtractable<std::complex<double>,const double>::value));
	BOOST_CHECK((is_subtractable<int,int>::value));
	BOOST_CHECK((is_subtractable<int,double>::value));
	BOOST_CHECK((is_subtractable<double,int>::value));
	BOOST_CHECK((is_subtractable<std::complex<double>,double>::value));
	BOOST_CHECK((is_subtractable<double,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<trivial,std::complex<double>>::value));
// Same as above.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK((!is_subtractable<int,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<std::complex<double>,int>::value));
#endif
	BOOST_CHECK((!is_subtractable<std::string,std::string>::value));
	BOOST_CHECK((!is_subtractable<std::string,const char *>::value));
	BOOST_CHECK((!is_subtractable<const char *,std::string>::value));
	BOOST_CHECK((is_subtractable<int *,std::size_t>::value));
	BOOST_CHECK((!is_subtractable<std::size_t,int *>::value));
	BOOST_CHECK(is_subtractable<int *>::value);
	BOOST_CHECK(is_subtractable<int &>::value);
	BOOST_CHECK((is_subtractable<int &, double &>::value));
	BOOST_CHECK((is_subtractable<double &, int &>::value));
	BOOST_CHECK(is_subtractable<int const &>::value);
	BOOST_CHECK((is_subtractable<int const &, double &>::value));
	BOOST_CHECK((is_subtractable<double const &, int &>::value));
	BOOST_CHECK(is_subtractable<int &&>::value);
	BOOST_CHECK((is_subtractable<int &&, double &&>::value));
	BOOST_CHECK((is_subtractable<double &&, int &&>::value));
	BOOST_CHECK((!is_subtractable<int &&, std::string &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_subtractable_in_place_test)
{
	BOOST_CHECK((is_subtractable_in_place<int>::value));
	BOOST_CHECK((is_subtractable_in_place<int,int>::value));
	BOOST_CHECK((is_subtractable_in_place<int,double>::value));
	BOOST_CHECK((is_subtractable_in_place<double,int>::value));
	BOOST_CHECK((is_subtractable_in_place<std::complex<double>,double>::value));
	BOOST_CHECK((!is_subtractable_in_place<double,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable_in_place<trivial,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable_in_place<std::string,std::string>::value));
	BOOST_CHECK((is_subtractable_in_place<int, const int>::value));
	BOOST_CHECK((!is_subtractable_in_place<const int, int>::value));
	BOOST_CHECK((!is_subtractable_in_place<const int &, int>::value));
	BOOST_CHECK((is_subtractable_in_place<int &&, const int &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_multipliable_test)
{
	BOOST_CHECK(is_multipliable<int>::value);
	BOOST_CHECK(is_multipliable<const int>::value);
	BOOST_CHECK((is_multipliable<const int, int>::value));
	BOOST_CHECK((is_multipliable<int, const int>::value));
	BOOST_CHECK((is_multipliable<const int &, int &>::value));
	BOOST_CHECK((is_multipliable<int &&, const int &>::value));
	BOOST_CHECK(is_multipliable<double>::value);
	BOOST_CHECK(is_multipliable<std::complex<double>>::value);
	BOOST_CHECK((is_multipliable<const std::complex<double>,double>::value));
	BOOST_CHECK((is_multipliable<std::complex<double>,const double>::value));
	BOOST_CHECK((is_multipliable<int,int>::value));
	BOOST_CHECK((is_multipliable<int,double>::value));
	BOOST_CHECK((is_multipliable<double,int>::value));
	BOOST_CHECK((is_multipliable<std::complex<double>,double>::value));
	BOOST_CHECK((is_multipliable<double,std::complex<double>>::value));
	BOOST_CHECK((!is_multipliable<trivial,std::complex<double>>::value));
	BOOST_CHECK((!is_multipliable<int *,std::size_t>::value));
	BOOST_CHECK((!is_multipliable<std::size_t,int *>::value));
	BOOST_CHECK(!is_multipliable<int *>::value);
	BOOST_CHECK(is_multipliable<int &>::value);
	BOOST_CHECK((is_multipliable<int &, double &>::value));
	BOOST_CHECK((is_multipliable<double &, int &>::value));
	BOOST_CHECK(is_multipliable<int const &>::value);
	BOOST_CHECK((is_multipliable<int const &, double &>::value));
	BOOST_CHECK((is_multipliable<double const &, int &>::value));
	BOOST_CHECK(is_multipliable<int &&>::value);
	BOOST_CHECK((is_multipliable<int &&, double &&>::value));
	BOOST_CHECK((is_multipliable<double &&, int &&>::value));
	BOOST_CHECK((!is_multipliable<int * &,int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_multipliable_in_place_test)
{
	BOOST_CHECK((is_multipliable_in_place<int>::value));
	BOOST_CHECK((is_multipliable_in_place<int,int>::value));
	BOOST_CHECK((is_multipliable_in_place<int,double>::value));
	BOOST_CHECK((is_multipliable_in_place<double,int>::value));
	BOOST_CHECK((is_multipliable_in_place<std::complex<double>,double>::value));
	BOOST_CHECK((!is_multipliable_in_place<double,std::complex<double>>::value));
	BOOST_CHECK((!is_multipliable_in_place<trivial,std::complex<double>>::value));
	BOOST_CHECK((is_multipliable_in_place<int, const int>::value));
	BOOST_CHECK((!is_multipliable_in_place<const int, int>::value));
	BOOST_CHECK((!is_multipliable_in_place<const int &, int>::value));
	BOOST_CHECK((is_multipliable_in_place<int &&, const int &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_divisible_test)
{
	BOOST_CHECK(is_divisible<int>::value);
	BOOST_CHECK(is_divisible<const int>::value);
	BOOST_CHECK((is_divisible<const int, int>::value));
	BOOST_CHECK((is_divisible<int, const int>::value));
	BOOST_CHECK((is_divisible<const int &, int &>::value));
	BOOST_CHECK((is_divisible<int &&, const int &>::value));
	BOOST_CHECK(is_divisible<double>::value);
	BOOST_CHECK(is_divisible<std::complex<double>>::value);
	BOOST_CHECK((is_divisible<const std::complex<double>,double>::value));
	BOOST_CHECK((is_divisible<std::complex<double>,const double>::value));
	BOOST_CHECK((is_divisible<int,int>::value));
	BOOST_CHECK((is_divisible<int,double>::value));
	BOOST_CHECK((is_divisible<double,int>::value));
	BOOST_CHECK((is_divisible<std::complex<double>,double>::value));
	BOOST_CHECK((is_divisible<double,std::complex<double>>::value));
	BOOST_CHECK((!is_divisible<trivial,std::complex<double>>::value));
	BOOST_CHECK((!is_divisible<int *,std::size_t>::value));
	BOOST_CHECK((!is_divisible<std::size_t,int *>::value));
	BOOST_CHECK(!is_divisible<int *>::value);
	BOOST_CHECK(is_divisible<int &>::value);
	BOOST_CHECK((is_divisible<int &, double &>::value));
	BOOST_CHECK((is_divisible<double &, int &>::value));
	BOOST_CHECK(is_divisible<int const &>::value);
	BOOST_CHECK((is_divisible<int const &, double &>::value));
	BOOST_CHECK((is_divisible<double const &, int &>::value));
	BOOST_CHECK(is_divisible<int &&>::value);
	BOOST_CHECK((is_divisible<int &&, double &&>::value));
	BOOST_CHECK((is_divisible<double &&, int &&>::value));
	BOOST_CHECK((!is_divisible<int * &,int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_divisible_in_place_test)
{
	BOOST_CHECK((is_divisible_in_place<int>::value));
	BOOST_CHECK((is_divisible_in_place<int,int>::value));
	BOOST_CHECK((is_divisible_in_place<int,double>::value));
	BOOST_CHECK((is_divisible_in_place<double,int>::value));
	BOOST_CHECK((is_divisible_in_place<std::complex<double>,double>::value));
	BOOST_CHECK((!is_divisible_in_place<double,std::complex<double>>::value));
	BOOST_CHECK((!is_divisible_in_place<trivial,std::complex<double>>::value));
	BOOST_CHECK((is_divisible_in_place<int, const int>::value));
	BOOST_CHECK((!is_divisible_in_place<const int, int>::value));
	BOOST_CHECK((!is_divisible_in_place<const int &, int>::value));
	BOOST_CHECK((is_divisible_in_place<int &&, const int &>::value));
}

struct frob
{
	bool operator==(const frob &) const;
	bool operator!=(const frob &) const;
	bool operator<(const frob &) const;
	bool operator>(const frob &) const;
};

struct frob_nonconst
{
	bool operator==(const frob_nonconst &);
	bool operator!=(const frob_nonconst &);
	bool operator<(const frob_nonconst &);
	bool operator>(const frob_nonconst &);
};

struct frob_nonbool
{
	char operator==(const frob_nonbool &) const;
	char operator!=(const frob_nonbool &) const;
	char operator<(const frob_nonbool &) const;
	char operator>(const frob_nonbool &) const;
};

struct frob_void
{
	void operator==(const frob_nonbool &) const;
	void operator!=(const frob_nonbool &) const;
	void operator<(const frob_nonbool &) const;
	void operator>(const frob_nonbool &) const;
};

struct frob_copy
{
	int operator==(frob_copy) const;
	int operator!=(frob_copy) const;
	int operator<(frob_copy) const;
	int operator>(frob_copy) const;
};

struct frob_mix
{};

short operator==(const frob_mix &, frob_mix);
short operator!=(const frob_mix &, frob_mix);
short operator<(const frob_mix &, frob_mix);
short operator>(const frob_mix &, frob_mix);

struct frob_mix_wrong
{};

short operator==(frob_mix_wrong, frob_mix_wrong &);
short operator!=(frob_mix_wrong, frob_mix_wrong &);
short operator<(frob_mix_wrong, frob_mix_wrong &);
short operator>(frob_mix_wrong, frob_mix_wrong &);

struct frob_mix_not_ineq
{};

bool operator==(const frob_mix_not_ineq &, const frob_mix_not_ineq &);

struct frob_mix_not_eq
{};

bool operator!=(const frob_mix_not_eq &, const frob_mix_not_eq &);

BOOST_AUTO_TEST_CASE(type_traits_is_equality_comparable_test)
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

BOOST_AUTO_TEST_CASE(type_traits_is_less_than_comparable_test)
{
	BOOST_CHECK(is_less_than_comparable<int>::value);
	BOOST_CHECK((is_less_than_comparable<int, double>::value));
	BOOST_CHECK((is_less_than_comparable<double, int>::value));
	BOOST_CHECK(is_less_than_comparable<int &>::value);
	BOOST_CHECK((is_less_than_comparable<const int &, double &&>::value));
	BOOST_CHECK((is_less_than_comparable<double, int &>::value));
	BOOST_CHECK((is_less_than_comparable<int *>::value));
	BOOST_CHECK((is_less_than_comparable<int const *>::value));
	BOOST_CHECK((is_less_than_comparable<int const *, int *>::value));
	BOOST_CHECK((!is_less_than_comparable<int *, double *>::value));
	BOOST_CHECK((!is_less_than_comparable<int *, double *>::value));
	BOOST_CHECK((is_less_than_comparable<frob>::value));
	BOOST_CHECK((!is_less_than_comparable<frob_nonconst>::value));
	BOOST_CHECK((is_less_than_comparable<frob_nonbool>::value));
	BOOST_CHECK((!is_less_than_comparable<frob_void>::value));
	BOOST_CHECK((is_less_than_comparable<frob_copy>::value));
	BOOST_CHECK((is_less_than_comparable<frob_mix>::value));
	BOOST_CHECK((!is_less_than_comparable<frob_mix_wrong>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_greater_than_comparable_test)
{
	BOOST_CHECK(is_greater_than_comparable<int>::value);
	BOOST_CHECK((is_greater_than_comparable<int, double>::value));
	BOOST_CHECK((is_greater_than_comparable<double, int>::value));
	BOOST_CHECK(is_greater_than_comparable<int &>::value);
	BOOST_CHECK((is_greater_than_comparable<const int &, double &&>::value));
	BOOST_CHECK((is_greater_than_comparable<double, int &>::value));
	BOOST_CHECK((is_greater_than_comparable<int *>::value));
	BOOST_CHECK((is_greater_than_comparable<int const *>::value));
	BOOST_CHECK((is_greater_than_comparable<int const *, int *>::value));
	BOOST_CHECK((!is_greater_than_comparable<int *, double *>::value));
	BOOST_CHECK((!is_greater_than_comparable<int *, double *>::value));
	BOOST_CHECK((is_greater_than_comparable<frob>::value));
	BOOST_CHECK((!is_greater_than_comparable<frob_nonconst>::value));
	BOOST_CHECK((is_greater_than_comparable<frob_nonbool>::value));
	BOOST_CHECK((!is_greater_than_comparable<frob_void>::value));
	BOOST_CHECK((is_greater_than_comparable<frob_copy>::value));
	BOOST_CHECK((is_greater_than_comparable<frob_mix>::value));
	BOOST_CHECK((!is_greater_than_comparable<frob_mix_wrong>::value));
}

template <typename T>
struct iio_base {};

template <typename T>
struct iio_derived: iio_base<T> {};

template <typename T>
struct iio_derived2: iio_base<T>, std::vector<T> {};

template <typename ... Args>
struct variadic_iio1 {};

template <typename Arg0, typename ... Args>
struct variadic_iio2 {};

BOOST_AUTO_TEST_CASE(type_traits_is_instance_of_test)
{
	BOOST_CHECK((is_instance_of<std::vector<double>,std::vector>::value));
	BOOST_CHECK((is_instance_of<std::vector<int>,std::vector>::value));
	BOOST_CHECK((!is_instance_of<std::vector<int>,std::set>::value));
	BOOST_CHECK((is_instance_of<iio_base<int>,iio_base>::value));
	BOOST_CHECK((is_instance_of<iio_derived<int>,iio_base>::value));
	BOOST_CHECK((!is_instance_of<iio_base<int>,iio_derived>::value));
	BOOST_CHECK((is_instance_of<std::ostream,std::basic_ios>::value));
	BOOST_CHECK((!is_instance_of<iio_base<int>,iio_derived>::value));
	BOOST_CHECK((!is_instance_of<int,std::list>::value));
	BOOST_CHECK((is_instance_of<iio_derived2<int>,std::vector>::value));
	BOOST_CHECK((is_instance_of<iio_derived2<int>,iio_base>::value));
	BOOST_CHECK((is_instance_of<iio_derived2<int> &,iio_base>::value));
	BOOST_CHECK((is_instance_of<iio_derived2<int> &&,iio_base>::value));
	BOOST_CHECK((is_instance_of<iio_derived2<int> const &,iio_base>::value));
	BOOST_CHECK((is_instance_of<iio_derived2<int> const &,iio_base>::value));
	BOOST_CHECK((is_instance_of<std::complex<double>,std::complex>::value));
	BOOST_CHECK((is_instance_of<variadic_iio1<>,variadic_iio1>::value));
	BOOST_CHECK((is_instance_of<variadic_iio1<int>,variadic_iio1>::value));
	BOOST_CHECK((is_instance_of<variadic_iio1<int,double>,variadic_iio1>::value));
	// See the comments in the source.
#if defined(PIRANHA_COMPILER_IS_GCC) || defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK((is_instance_of<variadic_iio2<int>,variadic_iio2>::value));
	BOOST_CHECK((is_instance_of<variadic_iio2<int,double>,variadic_iio2>::value));
#endif
}

struct stream1 {};

std::ostream &operator<<(std::ostream &, const stream1 &);

struct stream2 {};

std::ostream &operator<<(std::ostream &, stream2);

struct stream3 {};

std::ostream &operator<<(std::ostream &, stream3 &);

struct stream4 {};

void operator<<(std::ostream &, const stream4 &);

struct stream5 {};

std::ostream &operator<<(const std::ostream &, const stream5 &);

struct stream6 {};

const std::ostream &operator<<(std::ostream &, const stream6 &);

BOOST_AUTO_TEST_CASE(type_traits_is_ostreamable_test)
{
	BOOST_CHECK(is_ostreamable<int>::value);
	BOOST_CHECK(is_ostreamable<double>::value);
	BOOST_CHECK(is_ostreamable<int &>::value);
	BOOST_CHECK(is_ostreamable<double &&>::value);
	BOOST_CHECK(is_ostreamable<const int &>::value);
	BOOST_CHECK(!is_ostreamable<iio_base<int>>::value);
	BOOST_CHECK(is_ostreamable<stream1>::value);
	BOOST_CHECK(is_ostreamable<stream2>::value);
	BOOST_CHECK(!is_ostreamable<stream3>::value);
	BOOST_CHECK(!is_ostreamable<stream4>::value);
	BOOST_CHECK(is_ostreamable<stream5>::value);
	BOOST_CHECK(!is_ostreamable<stream6>::value);
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
	c_element2 &operator=(c_element2 &&) noexcept;
};

BOOST_AUTO_TEST_CASE(type_traits_is_container_element_test)
{
	BOOST_CHECK(is_container_element<int>::value);
	BOOST_CHECK(!is_container_element<int const>::value);
	BOOST_CHECK(is_container_element<double>::value);
	BOOST_CHECK(is_container_element<c_element>::value);
	BOOST_CHECK(!is_container_element<c_element const>::value);
	BOOST_CHECK(!is_container_element<c_element &>::value);
	BOOST_CHECK(!is_container_element<c_element const &>::value);
	BOOST_CHECK(!is_container_element<nc_element1>::value);
// Missing nothrow detection in the Intel compiler.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK(!is_container_element<nc_element2>::value);
#endif
	BOOST_CHECK(is_container_element<c_element2>::value);
	BOOST_CHECK(!is_container_element<int &>::value);
	BOOST_CHECK(!is_container_element<int &&>::value);
	BOOST_CHECK(!is_container_element<int const &>::value);
}

struct unhashable1 {};

struct unhashable2 {};

struct unhashable3 {};

struct unhashable4 {};

struct unhashable5 {};

struct unhashable6 {};

struct unhashable7 {};

struct unhashable8 {};

struct unhashable9 {};

struct unhashable10 {};

struct unhashable11 {};

struct unhashable12 {};

struct hashable1 {};

struct hashable2 {};

struct hashable3 {};

struct hashable4 {};

namespace std
{

template <>
struct hash<unhashable2>
{};

template <>
struct hash<unhashable3>
{
	int operator()(const unhashable3 &);
};

template <>
struct hash<unhashable4>
{
	std::size_t operator()(unhashable4 &);
};

template <>
struct hash<unhashable5>
{
	hash(const hash &) = delete;
	std::size_t operator()(const unhashable5 &);
};

template <>
struct hash<unhashable6>
{
	~hash() = delete;
	std::size_t operator()(const unhashable6 &);
};

template <>
struct hash<unhashable7>
{
	std::size_t operator()(const unhashable7 &);
};

template <>
struct hash<unhashable8>
{
	hash();
	std::size_t operator()(const unhashable8 &) noexcept;
};

template <>
struct hash<unhashable9>
{
	std::size_t operator()(const unhashable9 &) noexcept;
};

template <>
struct hash<unhashable10>
{
	std::size_t operator()(const unhashable10 &) const noexcept;
	~hash() noexcept(false) {}
};

template <>
struct hash<unhashable11>
{
	std::size_t operator()(const unhashable11 &) const noexcept;
	hash() noexcept;
	hash(const hash &);
	hash(hash &&) noexcept(false);
	hash &operator=(hash &&) noexcept;
	~hash();
};

template <>
struct hash<unhashable12>
{
	std::size_t operator()(const unhashable12 &) const noexcept;
	hash() noexcept;
	hash(const hash &);
	hash(hash &&) noexcept;
	hash &operator=(hash &&) noexcept(false);
	~hash();
};

template <>
struct hash<hashable1>
{
	std::size_t operator()(const hashable1 &) const noexcept;
};

template <>
struct hash<hashable2>
{
	std::size_t operator()(const hashable2 &) const noexcept;
};

template <>
struct hash<hashable3>
{
	hash() noexcept;
	std::size_t operator()(const hashable3 &) const noexcept;
};

template <>
struct hash<hashable4>
{
	std::size_t operator()(const hashable4 &) const noexcept;
	~hash() {}
};

}

BOOST_AUTO_TEST_CASE(type_traits_is_hashable_test)
{
	BOOST_CHECK(is_hashable<int>::value);
	BOOST_CHECK(is_hashable<std::string>::value);
	BOOST_CHECK(is_hashable<double>::value);
	BOOST_CHECK(is_hashable<double &>::value);
	BOOST_CHECK(is_hashable<double &&>::value);
	BOOST_CHECK(is_hashable<const double &>::value);
	BOOST_CHECK(is_hashable<const double>::value);
	// This is gonna fail on GCC 4.7.2 at least, depending on whether static_assert() is used
	// in the default implementation of the hasher.
	// http://stackoverflow.com/questions/16302977/static-assertions-and-sfinae
	// BOOST_CHECK(!is_hashable<unhashable1>::value);
	BOOST_CHECK(is_hashable<unhashable1 *>::value);
	BOOST_CHECK(is_hashable<unhashable1 const *>::value);
	BOOST_CHECK(!is_hashable<unhashable2>::value);
	BOOST_CHECK(!is_hashable<unhashable3>::value);
	BOOST_CHECK(!is_hashable<unhashable4>::value);
	BOOST_CHECK(!is_hashable<unhashable5>::value);
	BOOST_CHECK(!is_hashable<unhashable6>::value);
	BOOST_CHECK(!is_hashable<unhashable7>::value);
	BOOST_CHECK(!is_hashable<unhashable8>::value);
	BOOST_CHECK(!is_hashable<unhashable9>::value);
// Missing noexcept detect.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK(!is_hashable<unhashable10>::value);
	BOOST_CHECK(!is_hashable<unhashable11>::value);
	BOOST_CHECK(!is_hashable<unhashable12>::value);
#endif
	BOOST_CHECK(is_hashable<hashable1>::value);
	BOOST_CHECK(is_hashable<hashable2>::value);
	BOOST_CHECK(is_hashable<hashable2 &>::value);
	BOOST_CHECK(is_hashable<hashable2 &&>::value);
	BOOST_CHECK(is_hashable<hashable2 const &>::value);
	BOOST_CHECK(is_hashable<hashable2 *>::value);
	BOOST_CHECK(is_hashable<hashable2 const *>::value);
	BOOST_CHECK(is_hashable<hashable3>::value);
	BOOST_CHECK(is_hashable<hashable4>::value);
}

struct fo1 {};

struct fo2
{
	void operator()();
	void operator()() const;
};

struct fo3
{
	void operator()(int);
};

struct fo4
{
	void operator()(int);
	std::string operator()(int,double &);
};

struct fo5
{
	template <typename ... Args>
	int operator()(Args && ...);
};

struct fo6
{
	int operator()(int, int = 0);
};

void not_fo();

struct l5
{
	std::string &operator()(int &);
};

struct l6
{
	std::string const &operator()(int &);
};

BOOST_AUTO_TEST_CASE(type_traits_is_function_object_test)
{
	// NOTE: regarding lambdas:
	// http://en.cppreference.com/w/cpp/language/lambda
	// Specifically, they are always function objects and they have defaulted constructors.
	auto l1 = [](){};
	auto l2 = [](const int &){};
	auto l3 = [](int &){};
	auto l4 = [](int &) {return std::string{};};
	BOOST_CHECK((!is_function_object<int,void>::value));
	BOOST_CHECK((is_function_object<decltype(l1),void>::value));
	BOOST_CHECK((is_function_object<const decltype(l1),void>::value));
	BOOST_CHECK((!is_function_object<decltype(l1),void,int>::value));
	BOOST_CHECK((!is_function_object<const decltype(l2),void>::value));
	BOOST_CHECK((is_function_object<decltype(l2),void,int>::value));
	BOOST_CHECK((is_function_object<const decltype(l2),void,int &>::value));
	BOOST_CHECK((is_function_object<const decltype(l2),void,const int &>::value));
	BOOST_CHECK((!is_function_object<decltype(l3),void>::value));
	BOOST_CHECK((!is_function_object<const decltype(l3),void,int>::value));
	BOOST_CHECK((is_function_object<decltype(l3),void,int &>::value));
	BOOST_CHECK((!is_function_object<const decltype(l3),void,const int &>::value));
	BOOST_CHECK((!is_function_object<decltype(l3) &,void,int &>::value));
	BOOST_CHECK((!is_function_object<decltype(l3) const &,void,int &>::value));
	BOOST_CHECK((is_function_object<decltype(l4),std::string,int &>::value));
	BOOST_CHECK((!is_function_object<decltype(l4),std::string &,int &>::value));
	BOOST_CHECK((!is_function_object<l5,std::string,int &>::value));
	BOOST_CHECK((is_function_object<l5,std::string &,int &>::value));
	BOOST_CHECK((!is_function_object<l5,std::string const &,int &>::value));
	BOOST_CHECK((!is_function_object<l6,std::string,int &>::value));
	BOOST_CHECK((!is_function_object<l6,std::string &,int &>::value));
	BOOST_CHECK((is_function_object<l6,std::string const &,int &>::value));
	BOOST_CHECK((is_function_object<std::hash<int>,std::size_t,int>::value));
	BOOST_CHECK((is_function_object<const std::hash<int>,std::size_t,int &&>::value));
	BOOST_CHECK((is_function_object<const std::hash<int>,std::size_t,const int &>::value));
	BOOST_CHECK((is_function_object<const std::hash<int>,std::size_t,int &>::value));
	BOOST_CHECK((!is_function_object<const std::hash<int> &,std::size_t,int &>::value));
	BOOST_CHECK((!is_function_object<std::hash<int> &,std::size_t,int &>::value));
	BOOST_CHECK((!is_function_object<const std::hash<int>,int,int &>::value));
	BOOST_CHECK((!is_function_object<const std::hash<int>,std::size_t,int &, int &>::value));
	BOOST_CHECK((!is_function_object<const std::hash<int>,std::size_t>::value));
	BOOST_CHECK((!is_function_object<fo1,void>::value));
	BOOST_CHECK((!is_function_object<fo1,void,int>::value));
	BOOST_CHECK((is_function_object<fo2,void>::value));
	BOOST_CHECK((!is_function_object<fo2 *,void>::value));
	BOOST_CHECK((is_function_object<const fo2,void>::value));
	BOOST_CHECK((is_function_object<fo3,void,int>::value));
	BOOST_CHECK((!is_function_object<const fo3,void,int>::value));
	BOOST_CHECK((!is_function_object<fo3,void,int,int>::value));
	BOOST_CHECK((is_function_object<fo4,void,int>::value));
	BOOST_CHECK((is_function_object<fo4,std::string,int,double &>::value));
	BOOST_CHECK((!is_function_object<fo4,std::string,int,double &,int>::value));
	BOOST_CHECK((!is_function_object<fo4,std::string,int>::value));
	BOOST_CHECK((!is_function_object<fo4,std::string &,int,double &>::value));
	BOOST_CHECK((!is_function_object<fo4,std::string,int,double const &>::value));
	BOOST_CHECK((is_function_object<fo5,int>::value));
	BOOST_CHECK((is_function_object<fo5,int,double>::value));
	BOOST_CHECK((is_function_object<fo5,int,double,const std::string &>::value));
	BOOST_CHECK((!is_function_object<fo5,void,double,const std::string &>::value));
	BOOST_CHECK((is_function_object<fo6,int,int>::value));
	BOOST_CHECK((is_function_object<fo6,int,int,int>::value));
	BOOST_CHECK((!is_function_object<fo6,int,int,int,double>::value));
	BOOST_CHECK((!is_function_object<decltype(not_fo),void>::value));
	BOOST_CHECK((is_function_object<std::function<void(int)>,void,int>::value));
	BOOST_CHECK((!is_function_object<std::function<void(int)>,void>::value));
}

struct hfo1 {};

struct hfo2
{
	hfo2() noexcept;
	std::size_t operator()(int) noexcept;
};

struct hfo3
{
	hfo3() noexcept;
	std::size_t operator()(int) const noexcept;
};

struct hfo4
{
	hfo4() noexcept;
	std::size_t operator()(int) const noexcept;
	~hfo4() noexcept(false);
};

struct hfo5
{
	hfo5() noexcept;
	std::size_t operator()(int) const;
};

struct hfo6
{
	hfo6() noexcept;
	hfo6(const hfo6 &) = delete;
	std::size_t operator()(int) const noexcept;
};

struct hfo7
{
	hfo7() noexcept;
	std::size_t operator()(int) const noexcept;
	hfo7(const hfo7 &);
	hfo7(hfo7 &&) noexcept;
	hfo7 &operator=(hfo7 &&) noexcept;
};

struct hfo8
{
	hfo8() noexcept;
	std::size_t operator()(int) const noexcept;
	hfo8(const hfo7 &);
	hfo8(hfo8 &&) noexcept(false);
	hfo8 &operator=(hfo8 &&) noexcept;
};

struct hfo9
{
	hfo9() noexcept;
	std::size_t operator()(int) const noexcept;
	hfo9(const hfo9 &);
	hfo9(hfo9 &&) noexcept;
	hfo9 &operator=(hfo9 &&) noexcept(false);
};

BOOST_AUTO_TEST_CASE(type_traits_is_hash_function_object_test)
{
	BOOST_CHECK((is_hash_function_object<std::hash<int>,int>::value));
	BOOST_CHECK((is_hash_function_object<std::hash<int const *>,int const *>::value));
	BOOST_CHECK((is_hash_function_object<std::hash<int const *>,int *>::value));
	BOOST_CHECK((!is_hash_function_object<const std::hash<int const *>,int *>::value));
	BOOST_CHECK((!is_hash_function_object<std::hash<int> &,int &>::value));
	BOOST_CHECK((!is_hash_function_object<std::hash<int> const &,int &>::value));
	BOOST_CHECK((!is_hash_function_object<std::hash<int> &,const int &>::value));
	BOOST_CHECK((is_hash_function_object<std::hash<std::string>,std::string>::value));
	BOOST_CHECK((!is_hash_function_object<std::hash<int>,std::string>::value));
	BOOST_CHECK((!is_hash_function_object<int,int>::value));
	BOOST_CHECK((!is_hash_function_object<hfo1,int>::value));
	BOOST_CHECK((!is_hash_function_object<hfo2,int>::value));
	BOOST_CHECK((is_hash_function_object<hfo3,int>::value));
	BOOST_CHECK((is_hash_function_object<hfo3,short>::value));
	BOOST_CHECK((!is_hash_function_object<hfo4,int>::value));
	BOOST_CHECK((!is_hash_function_object<hfo5,int>::value));
	BOOST_CHECK((!is_hash_function_object<hfo6,int>::value));
	BOOST_CHECK((is_hash_function_object<hfo7,int>::value));
	BOOST_CHECK((!is_hash_function_object<hfo8,int>::value));
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK((!is_hash_function_object<hfo9,int>::value));
#endif
}

struct efo1 {};

struct efo2
{
	bool operator()(int,int) const;
};

struct efo3
{
	bool operator()(int,int);
};

struct efo4
{
	bool operator()(int,int) const;
	~efo4() noexcept(false);
};

struct efo5
{
	efo5() = delete;
	bool operator()(int,int) const;
};

struct efo6
{
	template <typename ... Args>
	bool operator()(Args && ...) const;
};

struct efo7
{
	efo7();
	efo7(const efo7 &);
	efo7(efo7 &&) noexcept;
	efo7 &operator=(const efo7 &);
	efo7 &operator=(efo7 &&) noexcept;
	bool operator()(int,int) const;
};

struct efo8
{
	efo8();
	efo8(const efo8 &);
	efo8(efo8 &&);
	efo8 &operator=(const efo8 &);
	efo8 &operator=(efo8 &&) noexcept;
	bool operator()(int,int) const;
};

struct efo9
{
	efo9();
	efo9(const efo9 &);
	efo9(efo9 &&) noexcept;
	efo9 &operator=(const efo9 &);
	efo9 &operator=(efo9 &&);
	bool operator()(int,int) const;
};

struct efo10
{
	bool operator()(int) const;
	bool operator()(int,int,int) const;
};

BOOST_AUTO_TEST_CASE(type_traits_is_equality_function_object_test)
{
	BOOST_CHECK((is_equality_function_object<std::equal_to<int>,int>::value));
	BOOST_CHECK((is_equality_function_object<std::equal_to<int>,short>::value));
	BOOST_CHECK((!is_equality_function_object<const std::equal_to<int>,short>::value));
	BOOST_CHECK((!is_equality_function_object<std::equal_to<int> &,short>::value));
	BOOST_CHECK((!is_equality_function_object<std::equal_to<int> &&,short>::value));
	BOOST_CHECK((!is_equality_function_object<std::hash<int>,int>::value));
	BOOST_CHECK((!is_equality_function_object<bool,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo1,int>::value));
	BOOST_CHECK((is_equality_function_object<efo2,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo3,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo4,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo5,int>::value));
	BOOST_CHECK((is_equality_function_object<efo6,int>::value));
	BOOST_CHECK((is_equality_function_object<efo6,std::string>::value));
	BOOST_CHECK((is_equality_function_object<efo7,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo7 const,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo7 const &,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo7 &,int>::value));
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK((!is_equality_function_object<efo8,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo9,int>::value));
#endif
	BOOST_CHECK((!is_equality_function_object<efo10,int>::value));
}

struct key01 {};

struct key02
{
	key02() = default;
	key02(const key02 &) = default;
	key02(key02 &&) noexcept;
	key02 &operator=(const key02 &) = default;
	key02 &operator=(key02 &&) noexcept;
	key02(const symbol_set &);
	bool operator==(const key02 &) const;
	bool operator!=(const key02 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key02 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key03
{
	key03() = default;
	key03(const key03 &) = default;
	key03(key03 &&) noexcept;
	key03 &operator=(const key03 &) = default;
	key03 &operator=(key03 &&) noexcept;
	key03(const symbol_set &);
	bool operator==(const key03 &) const;
	bool operator!=(const key03 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key03 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key04
{
	key04() = default;
	key04(const key04 &) = default;
	key04(key04 &&) noexcept(false);
	key04 &operator=(const key04 &) = default;
	key04 &operator=(key04 &&) noexcept;
	key04(const symbol_set &);
	bool operator==(const key04 &) const;
	bool operator!=(const key04 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key04 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key05
{
	key05() = default;
	key05(const key05 &) = default;
	key05(key05 &&) noexcept;
	key05 &operator=(const key05 &) = default;
	key05 &operator=(key05 &&) noexcept;
	key05(const symbol_set &);
	bool operator==(const key05 &) const;
	bool operator!=(const key05 &) const;
	bool is_compatible(const symbol_set &) const;
	bool is_ignorable(const symbol_set &) const noexcept;
	key05 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key06
{
	key06() = default;
	key06(const key06 &) = default;
	key06(key06 &&) noexcept;
	key06 &operator=(const key06 &) = default;
	key06 &operator=(key06 &&) noexcept;
	key06(const symbol_set &);
	bool operator==(const key06 &) const;
	bool operator!=(const key06 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const;
	key06 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key07
{
	key07() = default;
	key07(const key07 &) = default;
	key07(key07 &&) noexcept;
	key07 &operator=(const key07 &) = default;
	key07 &operator=(key07 &&) noexcept;
	key07(const symbol_set &);
	bool operator==(const key07 &) const;
	bool operator!=(const key07 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key07 merge_args(const symbol_set &, const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key08
{
	key08() = default;
	key08(const key08 &) = default;
	key08(key08 &&) noexcept;
	key08 &operator=(const key08 &) = default;
	key08 &operator=(key08 &&) noexcept;
	key08(const symbol_set &);
	bool operator==(const key08 &) const;
	bool operator!=(const key08 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key08 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

namespace std
{

template <>
struct hash<key02>
{
	std::size_t operator()(const key02 &) const noexcept;
};

template <>
struct hash<key03> {};

template <>
struct hash<key04>
{
	std::size_t operator()(const key04 &) const noexcept;
};

template <>
struct hash<key05>
{
	std::size_t operator()(const key05 &) const noexcept;
};

template <>
struct hash<key06>
{
	std::size_t operator()(const key06 &) const noexcept;
};

template <>
struct hash<key07>
{
	std::size_t operator()(const key07 &) const noexcept;
};

template <>
struct hash<key08>
{
	std::size_t operator()(const key08 &) const noexcept;
};

}

BOOST_AUTO_TEST_CASE(type_traits_is_key_test)
{
	BOOST_CHECK(!is_key<int>::value);
	BOOST_CHECK(!is_key<double>::value);
	BOOST_CHECK(!is_key<long *>::value);
	BOOST_CHECK(!is_key<long &>::value);
	BOOST_CHECK(!is_key<long const &>::value);
	BOOST_CHECK(!is_key<key01>::value);
	BOOST_CHECK(!is_key<const key01 &>::value);
	BOOST_CHECK(is_key<key02>::value);
	BOOST_CHECK(!is_key<key02 &>::value);
	BOOST_CHECK(!is_key<const key02>::value);
	BOOST_CHECK(!is_key<const key02 &>::value);
	BOOST_CHECK(!is_key<key03>::value);
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK(!is_key<key04>::value);
#endif
	BOOST_CHECK(!is_key<key05>::value);
	BOOST_CHECK(!is_key<key06>::value);
	BOOST_CHECK(!is_key<key07>::value);
	BOOST_CHECK(!is_key<key08>::value);
}

struct cf01 {};

struct cf02
{
	cf02();
	cf02(const int &);
	cf02(const cf02 &);
	cf02(cf02 &&) noexcept;
	cf02 &operator=(const cf02 &);
	cf02 &operator=(cf02 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const cf02 &);
	cf02 operator-() const;
	bool operator==(const cf02 &) const;
	bool operator!=(const cf02 &) const;
	cf02 &operator+=(const cf02 &);
	cf02 &operator-=(const cf02 &);
	cf02 operator+(const cf02 &) const;
	cf02 operator-(const cf02 &) const;
};

struct cf03
{
	cf03();
	cf03(const int &);
	cf03(const cf03 &);
	cf03(cf03 &&) noexcept;
	cf03 &operator=(const cf03 &);
	cf03 &operator=(cf03 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const cf03 &);
	bool operator==(const cf03 &) const;
	bool operator!=(const cf03 &) const;
	cf03 &operator+=(const cf03 &);
	cf03 &operator-=(const cf03 &);
	cf03 operator+(const cf03 &) const;
	cf03 operator-(const cf03 &) const;
};

struct cf04
{
	cf04();
	cf04(const int &);
	cf04(const cf04 &);
	cf04(cf04 &&) noexcept;
	cf04 &operator=(const cf04 &);
	cf04 &operator=(cf04 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const cf04 &);
	cf04 operator-() const;
	cf04 &operator+=(const cf04 &);
	cf04 &operator-=(const cf04 &);
	cf04 operator+(const cf04 &) const;
	cf04 operator-(const cf04 &) const;
};

struct cf05
{
	cf05();
	cf05(const cf05 &);
	cf05(cf05 &&) noexcept;
	cf05 &operator=(const cf05 &);
	cf05 &operator=(cf05 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const cf05 &);
	cf05 operator-() const;
	bool operator==(const cf05 &) const;
	bool operator!=(const cf05 &) const;
	cf05 &operator+=(const cf05 &);
	cf05 &operator-=(const cf05 &);
	cf05 operator+(const cf05 &) const;
	cf05 operator-(const cf05 &) const;
};

struct cf06
{
	cf06();
	cf06(const int &);
	cf06(const cf06 &);
	cf06(cf06 &&) noexcept(false);
	cf06 &operator=(const cf06 &);
	cf06 &operator=(cf06 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const cf06 &);
	cf06 operator-() const;
	bool operator==(const cf06 &) const;
	bool operator!=(const cf06 &) const;
	cf06 &operator+=(const cf06 &);
	cf06 &operator-=(const cf06 &);
	cf06 operator+(const cf06 &) const;
	cf06 operator-(const cf06 &) const;
};

struct cf07
{
	cf07();
	cf07(const int &);
	cf07(const cf07 &);
	cf07(cf07 &&) noexcept(false);
	cf07 &operator=(const cf07 &);
	cf07 &operator=(cf07 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const cf07 &);
	cf07 operator-() const;
	bool operator==(const cf07 &) const;
	bool operator!=(const cf07 &) const;
	cf07 &operator+=(const cf07 &);
	cf07 &operator-=(const cf07 &);
	cf07 operator+(const cf07 &) const;
	cf07 operator-(const cf07 &) const;
};

namespace piranha
{

template <typename T>
struct enable_noexcept_checks<T,typename std::enable_if<std::is_same<cf07,T>::value>::type>
{
	static const bool value = false;
};

template <typename T>
const bool enable_noexcept_checks<T,typename std::enable_if<std::is_same<cf07,T>::value>::type>::value;

}

BOOST_AUTO_TEST_CASE(type_traits_is_cf_test)
{
	BOOST_CHECK(is_cf<int>::value);
	BOOST_CHECK(is_cf<double>::value);
	BOOST_CHECK(is_cf<long double>::value);
	BOOST_CHECK(is_cf<std::complex<double>>::value);
	// NOTE: the checks on the pointers here produce warnings in clang. The reason is that
	// ptr1 -= ptr2
	// expands to
	// ptr1 = ptr1 - ptr2
	// where ptr1 - ptr2 is some integer value (due to pointer arith.) that then gets assigned back to a pointer. It is not
	// entirely clear if this should be a hard error (GCC) or just a warning (clang) so for now it is better to simply disable
	// the check. Note that the same problem would be in is_subtractable_in_place if we checked for pointers there.
	// BOOST_CHECK(!is_cf<double *>::value);
	// BOOST_CHECK(!is_cf<double const *>::value);
	BOOST_CHECK(!is_cf<int &>::value);
	BOOST_CHECK(!is_cf<int const &>::value);
	BOOST_CHECK(!is_cf<int const &>::value);
	BOOST_CHECK(!is_cf<cf01>::value);
	BOOST_CHECK(is_cf<cf02>::value);
	// BOOST_CHECK(!is_cf<cf02 *>::value);
	BOOST_CHECK(!is_cf<cf02 &&>::value);
	BOOST_CHECK(!is_cf<cf03>::value);
	BOOST_CHECK(!is_cf<cf04>::value);
	BOOST_CHECK(!is_cf<cf05>::value);
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK(!is_cf<cf06>::value);
#endif
	BOOST_CHECK(is_cf<cf07>::value);
}

struct term_type1: base_term<double,monomial<int>,term_type1>
{
	typedef base_term<double,monomial<int>,term_type1> base;
	PIRANHA_FORWARDING_CTOR(term_type1,base)
	term_type1() = default;
	term_type1(const term_type1 &) = default;
	term_type1(term_type1 &&) = default;
	term_type1 &operator=(const term_type1 &) = default;
	term_type1 &operator=(term_type1 &&) = default;
};

struct term_type2: base_term<double,monomial<int>,term_type2>
{
	typedef base_term<double,monomial<int>,term_type2> base;
	PIRANHA_FORWARDING_CTOR(term_type2,base)
	term_type2() = default;
	term_type2(const term_type2 &) = default;
	term_type2(term_type2 &&) = default;
	term_type2 &operator=(const term_type2 &) = default;
	term_type2 &operator=(term_type2 &&) = default;
	typedef term_type2 multiplication_result_type;
	void multiply(term_type2 &, const term_type2 &, const symbol_set &) const;
};

struct term_type3: base_term<double,monomial<int>,term_type3>
{
	typedef base_term<double,monomial<int>,term_type3> base;
	PIRANHA_FORWARDING_CTOR(term_type3,base)
	term_type3() = default;
	term_type3(const term_type3 &) = default;
	term_type3(term_type3 &&) = default;
	term_type3 &operator=(const term_type3 &) = default;
	term_type3 &operator=(term_type3 &&) = default;
	typedef term_type3 multiplication_result_type;
	void multiply(term_type3 &, const term_type3 &, const symbol_set &);
};

struct term_type4: base_term<double,monomial<int>,term_type4>
{
	typedef base_term<double,monomial<int>,term_type4> base;
	PIRANHA_FORWARDING_CTOR(term_type4,base)
	term_type4() = default;
	term_type4(const term_type4 &) = default;
	term_type4(term_type4 &&) = default;
	term_type4 &operator=(const term_type4 &) = default;
	term_type4 &operator=(term_type4 &&) = default;
	typedef std::tuple<term_type4> multiplication_result_type;
	void multiply(multiplication_result_type &, const term_type4 &, const symbol_set &) const;
};

struct term_type5: base_term<double,monomial<int>,term_type5>
{
	typedef base_term<double,monomial<int>,term_type5> base;
	PIRANHA_FORWARDING_CTOR(term_type5,base)
	term_type5() = default;
	term_type5(const term_type5 &) = default;
	term_type5(term_type5 &&) = default;
	term_type5 &operator=(const term_type5 &) = default;
	term_type5 &operator=(term_type5 &&) = default;
	typedef std::tuple<> multiplication_result_type;
	void multiply(multiplication_result_type &, const term_type5 &, const symbol_set &) const;
};

struct term_type6: base_term<double,monomial<int>,term_type6>
{
	typedef base_term<double,monomial<int>,term_type6> base;
	PIRANHA_FORWARDING_CTOR(term_type6,base)
	term_type6() = default;
	term_type6(const term_type6 &) = default;
	term_type6(term_type6 &&) = default;
	term_type6 &operator=(const term_type6 &) = default;
	term_type6 &operator=(term_type6 &&) = default;
	typedef std::tuple<term_type6,term_type6> multiplication_result_type;
	void multiply(multiplication_result_type &, const term_type6 &, const symbol_set &) const;
};

struct term_type7: base_term<double,monomial<int>,term_type7>
{
	typedef base_term<double,monomial<int>,term_type7> base;
	PIRANHA_FORWARDING_CTOR(term_type7,base)
	term_type7() = default;
	term_type7(const term_type7 &) = default;
	term_type7(term_type7 &&) = default;
	term_type7 &operator=(const term_type7 &) = default;
	term_type7 &operator=(term_type7 &&) = default;
	typedef std::tuple<term_type7,int> multiplication_result_type;
	void multiply(multiplication_result_type &, const term_type7 &, const symbol_set &) const;
};

struct term_type8: base_term<double,monomial<int>,term_type8>
{
	typedef base_term<double,monomial<int>,term_type8> base;
	PIRANHA_FORWARDING_CTOR(term_type8,base)
	term_type8() = default;
	term_type8(const term_type8 &) = default;
	term_type8(term_type8 &&) = default;
	term_type8 &operator=(const term_type8 &) = default;
	term_type8 &operator=(term_type8 &&) = default;
	typedef std::tuple<term_type8,term_type8> multiplication_result_type;
	void multiply(multiplication_result_type &, term_type8 &, const symbol_set &) const;
};

BOOST_AUTO_TEST_CASE(type_traits_term_is_multipliable_test)
{
	BOOST_CHECK(!term_is_multipliable<term_type1>::value);
	BOOST_CHECK(term_is_multipliable<term_type2>::value);
	BOOST_CHECK(!term_is_multipliable<term_type3>::value);
	BOOST_CHECK(term_is_multipliable<term_type4>::value);
	BOOST_CHECK(!term_is_multipliable<term_type5>::value);
	BOOST_CHECK(term_is_multipliable<term_type6>::value);
	BOOST_CHECK(!term_is_multipliable<term_type7>::value);
	BOOST_CHECK(!term_is_multipliable<term_type8>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_min_max_int_test)
{
	BOOST_CHECK((std::is_same<int,min_int<int>>::value));
	BOOST_CHECK((std::is_same<unsigned,min_int<unsigned>>::value));
	BOOST_CHECK((std::is_same<int,max_int<int>>::value));
	BOOST_CHECK((std::is_same<unsigned,max_int<unsigned>>::value));
	BOOST_CHECK((std::is_same<int,max_int<short, int>>::value));
	BOOST_CHECK((std::is_same<unsigned,max_int<unsigned short, unsigned>>::value));
	if (boost::integer_traits<long long>::const_max > boost::integer_traits<int>::const_max &&
		boost::integer_traits<long long>::const_min < boost::integer_traits<int>::const_min)
	{
		BOOST_CHECK((std::is_same<long long,max_int<short, int, signed char, long long>>::value));
		BOOST_CHECK((std::is_same<long long,max_int<long long, int, signed char, short>>::value));
		BOOST_CHECK((std::is_same<long long,max_int<int, long long, signed char, short>>::value));
		BOOST_CHECK((std::is_same<long long,max_int<short, signed char, long long, int>>::value));
	}
	if (boost::integer_traits<unsigned long long>::const_max > boost::integer_traits<unsigned>::const_max) {
		BOOST_CHECK((std::is_same<unsigned long long,max_int<unsigned short, unsigned, unsigned char, unsigned long long>>::value));
		BOOST_CHECK((std::is_same<unsigned long long,max_int<unsigned long long, unsigned, unsigned char, unsigned short>>::value));
		BOOST_CHECK((std::is_same<unsigned long long,max_int<unsigned, unsigned long long, unsigned char, unsigned short>>::value));
		BOOST_CHECK((std::is_same<unsigned long long,max_int<unsigned short, unsigned char, unsigned long long, unsigned>>::value));
	}
	if (boost::integer_traits<signed char>::const_max < boost::integer_traits<short>::const_max &&
		boost::integer_traits<signed char>::const_min > boost::integer_traits<short>::const_min)
	{
		BOOST_CHECK((std::is_same<signed char,min_int<short, int, signed char, long long>>::value));
		BOOST_CHECK((std::is_same<signed char,min_int<long long, int, signed char, short>>::value));
		BOOST_CHECK((std::is_same<signed char,min_int<int, long long, signed char, short>>::value));
		BOOST_CHECK((std::is_same<signed char,min_int<short, signed char, long long, int>>::value));
	}
	if (boost::integer_traits<unsigned char>::const_min < boost::integer_traits<unsigned short>::const_max) {
		BOOST_CHECK((std::is_same<unsigned char,min_int<unsigned short, unsigned, unsigned char, unsigned long long>>::value));
		BOOST_CHECK((std::is_same<unsigned char,min_int<unsigned long long, unsigned, unsigned char, unsigned short>>::value));
		BOOST_CHECK((std::is_same<unsigned char,min_int<unsigned, unsigned long long, unsigned char, unsigned short>>::value));
		BOOST_CHECK((std::is_same<unsigned char,min_int<unsigned short, unsigned char, unsigned long long, unsigned>>::value));
	}
}

// Boilerplate to test the arrow op type trait.
template <typename T>
using aot = detail::arrow_operator_type<T>;

PIRANHA_DECLARE_HAS_TYPEDEF(type);

struct arrow01
{
	int *operator->();
};

struct arrow02
{
	arrow01 operator->();
};

struct arrow03
{
	int operator->();
};

struct arrow03a
{
	arrow02 operator->();
};

struct arrow04
{
	arrow03 operator->();
};

// Good forward iterator.
template <typename T>
struct fake_it_traits_forward
{
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = T *;
	using reference = T &;
	using iterator_category = std::forward_iterator_tag;
};

// Broken reference type for forward it.
template <typename T>
struct fake_it_traits_forward_broken_ref
{
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = T *;
	using reference = void;
	using iterator_category = std::forward_iterator_tag;
};

// Good output iterator.
template <typename T>
struct fake_it_traits_output
{
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = T *;
	using reference = T &;
	using iterator_category = std::output_iterator_tag;
};

// Good input iterator.
template <typename T>
struct fake_it_traits_input
{
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = T *;
	using reference = T &;
	using iterator_category = std::input_iterator_tag;
};

// Broken trait, incorrect category.
template <typename T>
struct fake_it_traits_wrong_tag
{
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = T *;
	using reference = T &;
	using iterator_category = void;
};

// Broken trait, missing typedefs.
template <typename T>
struct fake_it_traits_missing
{
	using value_type = void;
	using pointer = void;
	using iterator_category = void;
};

#define PIRANHA_DECL_ITT_SPEC(it_type,trait_class) \
namespace std \
{ \
template <> \
struct iterator_traits<it_type>: trait_class {}; \
}

// Good input iterator.
struct iter01
{
	int &operator*();
	int *operator->();
	iter01 &operator++();
	iter01 &operator++(int);
	bool operator==(const iter01 &) const;
	bool operator!=(const iter01 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter01,fake_it_traits_input<int>)

// Good iterator, minimal requirements.
struct iter02
{
	int &operator*();
	iter02 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter02,fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter03
{
	//int &operator*();
	iter03 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter03,fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter04
{
	~iter04() = delete;
	int &operator*();
	iter04 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter04,fake_it_traits_input<int>)

// Broken iterator, missing itt spec.
struct iter05
{
	int &operator*();
	iter05 &operator++();
};

//PIRANHA_DECL_ITT_SPEC(iter05,fake_it_traits_input<int>)

// Broken input iterator: missing arrow.
struct iter06
{
	int &operator*();
	//int *operator->();
	iter06 &operator++();
	iter06 &operator++(int);
	bool operator==(const iter06 &) const;
	bool operator!=(const iter06 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter06,fake_it_traits_input<int>)

// Broken input iterator: missing equality.
struct iter07
{
	int &operator*();
	int *operator->();
	iter07 &operator++();
	iter07 &operator++(int);
	//bool operator==(const iter07 &) const;
	bool operator!=(const iter07 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter07,fake_it_traits_input<int>)

// Broken input iterator: missing itt spec.
struct iter08
{
	int &operator*();
	int *operator->();
	iter08 &operator++();
	iter08 &operator++(int);
	bool operator==(const iter08 &) const;
	bool operator!=(const iter08 &) const;
};

//PIRANHA_DECL_ITT_SPEC(iter08,fake_it_traits_input<int>)

// Broken input iterator: broken arrow.
struct iter09
{
	int &operator*();
	int operator->();
	iter09 &operator++();
	iter09 &operator++(int);
	bool operator==(const iter09 &) const;
	bool operator!=(const iter09 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter09,fake_it_traits_input<int>)

// Good input iterator: multiple arrow.
struct iter10
{
	int &operator*();
	arrow03a operator->();
	iter10 &operator++();
	iter10 &operator++(int);
	bool operator==(const iter10 &) const;
	bool operator!=(const iter10 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter10,fake_it_traits_input<int>)

// Bad input iterator: multiple broken arrow.
struct iter11
{
	int &operator*();
	arrow04 operator->();
	iter11 &operator++();
	iter11 &operator++(int);
	bool operator==(const iter11 &) const;
	bool operator!=(const iter11 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter11,fake_it_traits_input<int>)

// Bad input iterator: inconsistent arrow / star.
struct foo_it_12 {};

struct iter12
{
	int &operator*();
	foo_it_12 *operator->();
	iter12 &operator++();
	iter12 &operator++(int);
	bool operator==(const iter12 &) const;
	bool operator!=(const iter12 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter12,fake_it_traits_input<int>)

// Good input iterator: different but compatible arrow / star.
struct iter13
{
	int operator*();
	int *operator->();
	iter13 &operator++();
	iter13 &operator++(int);
	bool operator==(const iter13 &) const;
	bool operator!=(const iter13 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter13,fake_it_traits_input<int>)

// Good forward iterator.
struct iter14
{
	int &operator*();
	int *operator->();
	iter14 &operator++();
	iter14 &operator++(int);
	bool operator==(const iter14 &) const;
	bool operator!=(const iter14 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter14,fake_it_traits_forward<int>)

// Bad forward iterator: missing def ctor.
struct iter15
{
	iter15() = delete;
	int &operator*();
	int *operator->();
	iter15 &operator++();
	iter15 &operator++(int);
	bool operator==(const iter15 &) const;
	bool operator!=(const iter15 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter15,fake_it_traits_forward<int>)

// Bad forward iterator: not having reference types as reference in traits.
struct iter16
{
	int &operator*();
	int *operator->();
	iter16 &operator++();
	iter16 &operator++(int);
	bool operator==(const iter16 &) const;
	bool operator!=(const iter16 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter16,fake_it_traits_forward_broken_ref<int>)

// Bad forward iterator: broken tag in traits.
struct iter17
{
	int &operator*();
	int *operator->();
	iter17 &operator++();
	iter17 &operator++(int);
	bool operator==(const iter17 &) const;
	bool operator!=(const iter17 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter17,fake_it_traits_output<int>)

// Bad forward iterator: broken traits.
struct iter18
{
	int &operator*();
	int *operator->();
	iter18 &operator++();
	iter18 &operator++(int);
	bool operator==(const iter18 &) const;
	bool operator!=(const iter18 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter18,fake_it_traits_missing<int>)

// Bad forward iterator: broken ++.
struct iter19
{
	int &operator*();
	int *operator->();
	iter19 &operator++();
	void operator++(int);
	bool operator==(const iter19 &) const;
	bool operator!=(const iter19 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter19,fake_it_traits_forward<int>)

// Bad forward iterator: broken ++.
struct iter20
{
	int &operator*();
	int *operator->();
	void operator++();
	iter20 &operator++(int);
	bool operator==(const iter20 &) const;
	bool operator!=(const iter20 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter20,fake_it_traits_forward<int>)

// Bad forward iterator: arrow returns type with different constness from star operator.
struct iter21
{
	int &operator*();
	const int *operator->();
	iter21 &operator++();
	iter21 &operator++(int);
	bool operator==(const iter21 &) const;
	bool operator!=(const iter21 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter21,fake_it_traits_forward<int>)

#undef PIRANHA_DECL_ITT_SPEC

BOOST_AUTO_TEST_CASE(type_traits_iterator_test)
{
	// Check the arrow operator type trait in detail::.
	BOOST_CHECK(has_typedef_type<aot<int *>>::value);
	BOOST_CHECK((std::is_same<typename aot<int *>::type,int *>::value));
	BOOST_CHECK(!has_typedef_type<aot<int>>::value);
	BOOST_CHECK(has_typedef_type<aot<arrow01>>::value);
	BOOST_CHECK((std::is_same<typename aot<arrow01>::type,int *>::value));
	BOOST_CHECK(has_typedef_type<aot<arrow02>>::value);
	BOOST_CHECK((std::is_same<typename aot<arrow02>::type,int *>::value));
	BOOST_CHECK(!has_typedef_type<aot<arrow03>>::value);
	BOOST_CHECK(has_typedef_type<aot<arrow03a>>::value);
	BOOST_CHECK((std::is_same<typename aot<arrow03a>::type,int *>::value));
	// Iterator.
	BOOST_CHECK(detail::has_iterator_traits<int *>::value);
	BOOST_CHECK(detail::has_iterator_traits<const int *>::value);
	BOOST_CHECK(!detail::has_iterator_traits<int>::value);
	BOOST_CHECK(!detail::has_iterator_traits<double>::value);
	BOOST_CHECK(detail::has_iterator_traits<std::vector<int>::iterator>::value);
	BOOST_CHECK(detail::has_iterator_traits<std::vector<int>::const_iterator>::value);
	BOOST_CHECK(is_iterator<int *>::value);
	BOOST_CHECK(is_iterator<const int *>::value);
	BOOST_CHECK(is_iterator<std::vector<int>::iterator>::value);
	BOOST_CHECK(is_iterator<std::vector<int>::const_iterator>::value);
	BOOST_CHECK(is_iterator<std::vector<int>::iterator &>::value);
	BOOST_CHECK(!is_iterator<int>::value);
	BOOST_CHECK(!is_iterator<std::string>::value);
	BOOST_CHECK(is_iterator<iter01>::value);
	BOOST_CHECK(is_iterator<iter01 &>::value);
	BOOST_CHECK(is_iterator<const iter01>::value);
	BOOST_CHECK(is_iterator<iter02>::value);
	BOOST_CHECK(is_iterator<iter02 &>::value);
	BOOST_CHECK(is_iterator<const iter02>::value);
	BOOST_CHECK(!is_iterator<iter03>::value);
	BOOST_CHECK(!is_iterator<iter03 &>::value);
	BOOST_CHECK(!is_iterator<const iter03>::value);
// The Intel compiler has problems with the destructible
// type-trait.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
	BOOST_CHECK(!is_iterator<iter04>::value);
	BOOST_CHECK(!is_iterator<iter04 &>::value);
	BOOST_CHECK(!is_iterator<const iter04>::value);
#endif
	BOOST_CHECK(!is_iterator<iter05>::value);
	BOOST_CHECK(!is_iterator<iter05 &>::value);
	BOOST_CHECK(!is_iterator<const iter05>::value);
	BOOST_CHECK(is_iterator<std::ostream_iterator<int>>::value);
	BOOST_CHECK(is_iterator<std::insert_iterator<std::list<int>>>::value);
	// Input iterator.
	BOOST_CHECK(is_input_iterator<int *>::value);
	BOOST_CHECK(is_input_iterator<const int *>::value);
	BOOST_CHECK(is_input_iterator<std::vector<int>::iterator>::value);
	BOOST_CHECK(is_input_iterator<std::vector<int>::const_iterator>::value);
	BOOST_CHECK(is_input_iterator<std::vector<int>::iterator &>::value);
	BOOST_CHECK(is_input_iterator<std::istream_iterator<double>>::value);
	BOOST_CHECK(is_input_iterator<iter01>::value);
	BOOST_CHECK(is_input_iterator<iter01 &>::value);
	BOOST_CHECK(is_input_iterator<const iter01>::value);
	BOOST_CHECK(!is_input_iterator<iter02>::value);
	BOOST_CHECK(!is_input_iterator<iter02 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter02>::value);
	BOOST_CHECK(!is_input_iterator<iter06>::value);
	BOOST_CHECK(!is_input_iterator<iter06 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter06>::value);
	BOOST_CHECK(is_iterator<iter06>::value);
	BOOST_CHECK(is_iterator<iter06 &>::value);
	BOOST_CHECK(is_iterator<const iter06>::value);
	BOOST_CHECK(!is_input_iterator<iter07>::value);
	BOOST_CHECK(!is_input_iterator<iter07 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter07>::value);
	BOOST_CHECK(is_iterator<iter07>::value);
	BOOST_CHECK(is_iterator<iter07 &>::value);
	BOOST_CHECK(is_iterator<const iter07>::value);
	BOOST_CHECK(!is_input_iterator<iter08>::value);
	BOOST_CHECK(!is_input_iterator<iter08 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter08>::value);
	BOOST_CHECK(!is_iterator<iter08>::value);
	BOOST_CHECK(!is_iterator<iter08 &>::value);
	BOOST_CHECK(!is_iterator<const iter08>::value);
	BOOST_CHECK(!is_input_iterator<iter09>::value);
	BOOST_CHECK(!is_input_iterator<iter09 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter09>::value);
	BOOST_CHECK(is_input_iterator<iter10>::value);
	BOOST_CHECK(is_input_iterator<iter10 &>::value);
	BOOST_CHECK(is_input_iterator<const iter10>::value);
	BOOST_CHECK(!is_input_iterator<iter11>::value);
	BOOST_CHECK(!is_input_iterator<iter11 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter11>::value);
	BOOST_CHECK(is_iterator<iter11>::value);
	BOOST_CHECK(is_iterator<iter11 &>::value);
	BOOST_CHECK(is_iterator<const iter11>::value);
	BOOST_CHECK(!is_input_iterator<iter12>::value);
	BOOST_CHECK(!is_input_iterator<iter12 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter12>::value);
	BOOST_CHECK(is_iterator<iter12>::value);
	BOOST_CHECK(is_iterator<iter12 &>::value);
	BOOST_CHECK(is_iterator<const iter12>::value);
	BOOST_CHECK(is_input_iterator<iter13>::value);
	BOOST_CHECK(is_input_iterator<iter13 &>::value);
	BOOST_CHECK(is_input_iterator<const iter13>::value);
	// Forward iterator.
	BOOST_CHECK(is_forward_iterator<int *>::value);
	BOOST_CHECK(is_forward_iterator<const int *>::value);
	BOOST_CHECK(is_forward_iterator<std::vector<int>::iterator>::value);
	BOOST_CHECK(is_forward_iterator<std::vector<int>::const_iterator>::value);
	BOOST_CHECK(is_forward_iterator<std::vector<int>::iterator &>::value);
	BOOST_CHECK(!is_forward_iterator<std::istream_iterator<double>>::value);
	BOOST_CHECK(is_forward_iterator<iter14>::value);
	BOOST_CHECK(is_forward_iterator<iter14 &>::value);
	BOOST_CHECK(is_forward_iterator<const iter14>::value);
	BOOST_CHECK(!is_forward_iterator<iter15>::value);
	BOOST_CHECK(!is_forward_iterator<iter15 &>::value);
	BOOST_CHECK(!is_forward_iterator<const iter15>::value);
	BOOST_CHECK(is_input_iterator<iter15>::value);
	BOOST_CHECK(is_input_iterator<iter15 &>::value);
	BOOST_CHECK(is_input_iterator<const iter15>::value);
	BOOST_CHECK(!is_forward_iterator<iter17>::value);
	BOOST_CHECK(!is_forward_iterator<iter17 &>::value);
	BOOST_CHECK(!is_forward_iterator<const iter17>::value);
	BOOST_CHECK(is_iterator<iter17>::value);
	BOOST_CHECK(is_iterator<iter17 &>::value);
	BOOST_CHECK(is_iterator<const iter17>::value);
	BOOST_CHECK(!is_forward_iterator<iter18>::value);
	BOOST_CHECK(!is_forward_iterator<iter18 &>::value);
	BOOST_CHECK(!is_forward_iterator<const iter18>::value);
	BOOST_CHECK(!is_iterator<iter18>::value);
	BOOST_CHECK(!is_iterator<iter18 &>::value);
	BOOST_CHECK(!is_iterator<const iter18>::value);
	BOOST_CHECK(!is_forward_iterator<iter19>::value);
	BOOST_CHECK(!is_forward_iterator<iter19 &>::value);
	BOOST_CHECK(!is_forward_iterator<const iter19>::value);
	BOOST_CHECK(!is_input_iterator<iter19>::value);
	BOOST_CHECK(!is_input_iterator<iter19 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter19>::value);
	BOOST_CHECK(!is_forward_iterator<iter20>::value);
	BOOST_CHECK(!is_forward_iterator<iter20 &>::value);
	BOOST_CHECK(!is_forward_iterator<const iter20>::value);
	BOOST_CHECK(!is_input_iterator<iter20>::value);
	BOOST_CHECK(!is_input_iterator<iter20 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter20>::value);
	BOOST_CHECK(!is_forward_iterator<iter21>::value);
	BOOST_CHECK(!is_forward_iterator<iter21 &>::value);
	BOOST_CHECK(!is_forward_iterator<const iter21>::value);
	BOOST_CHECK(!is_input_iterator<iter21>::value);
	BOOST_CHECK(!is_input_iterator<iter21 &>::value);
	BOOST_CHECK(!is_input_iterator<const iter21>::value);
	BOOST_CHECK(is_iterator<iter21>::value);
	BOOST_CHECK(is_iterator<iter21 &>::value);
	BOOST_CHECK(is_iterator<const iter21>::value);
}

template <typename S>
using sai = detail::safe_abs_sint<S>;

BOOST_AUTO_TEST_CASE(type_traits_safe_abs_sint_test)
{
	BOOST_CHECK(sai<signed char>::value > 1);
	BOOST_CHECK(sai<short>::value > 1);
	BOOST_CHECK(sai<int>::value > 1);
	BOOST_CHECK(sai<long>::value > 1);
	BOOST_CHECK(sai<long long>::value > 1);
}
