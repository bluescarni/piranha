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
#include <cstddef>
#include <functional>
#include <ios>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/symbol_set.hpp"

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

BOOST_AUTO_TEST_CASE(type_traits_is_trivially_copyable_test)
{
	BOOST_CHECK_EQUAL(is_trivially_copyable<int>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_copyable<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_copyable<nontrivial_dtor>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_copyable<nontrivial_copy>::value,false);
	BOOST_CHECK_EQUAL(is_trivially_copyable<std::string>::value,false);
}

BOOST_AUTO_TEST_CASE(type_traits_is_trivially_destructible_test)
{
	BOOST_CHECK_EQUAL(is_trivially_destructible<int>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_destructible<trivial>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_destructible<nontrivial_copy>::value,true);
	BOOST_CHECK_EQUAL(is_trivially_destructible<nontrivial_dtor>::value,false);
	BOOST_CHECK_EQUAL(is_trivially_destructible<std::string>::value,false);
}

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

BOOST_AUTO_TEST_CASE(type_traits_is_tuple_test)
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

BOOST_AUTO_TEST_CASE(type_traits_has_degree_test)
{
	BOOST_CHECK(!has_degree<int>::value);
	BOOST_CHECK(!has_degree<double>::value);
	BOOST_CHECK(has_degree<trivial>::value);
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
	BOOST_CHECK((!is_addable<int,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<std::complex<double>,int>::value));
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
	BOOST_CHECK((!is_subtractable<int,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<std::complex<double>,int>::value));
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

struct frob
{
	bool operator==(const frob &) const;
	bool operator!=(const frob &) const;
	bool operator<(const frob &) const;
};

struct frob_nonconst
{
	bool operator==(const frob_nonconst &);
	bool operator!=(const frob_nonconst &);
	bool operator<(const frob_nonconst &);
};

struct frob_nonbool
{
	char operator==(const frob_nonbool &) const;
	char operator!=(const frob_nonbool &) const;
	char operator<(const frob_nonbool &) const;
};

struct frob_void
{
	void operator==(const frob_nonbool &) const;
	void operator!=(const frob_nonbool &) const;
	void operator<(const frob_nonbool &) const;
};

struct frob_copy
{
	int operator==(frob_copy) const;
	int operator!=(frob_copy) const;
	int operator<(frob_copy) const;
};

struct frob_mix
{};

short operator==(const frob_mix &, frob_mix);
short operator!=(const frob_mix &, frob_mix);
short operator<(const frob_mix &, frob_mix);

struct frob_mix_wrong
{};

short operator==(frob_mix_wrong, frob_mix_wrong &);
short operator!=(frob_mix_wrong, frob_mix_wrong &);
short operator<(frob_mix_wrong, frob_mix_wrong &);

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

template <typename T>
struct iio_base {};

template <typename T>
struct iio_derived: iio_base<T> {};

template <typename T>
struct iio_derived2: iio_base<T>, std::vector<T> {};

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
	c_element2 &operator=(c_element2 &&) noexcept(true);
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
	BOOST_CHECK(!is_container_element<nc_element2>::value);
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
	std::size_t operator()(const unhashable8 &) noexcept(true);
};

template <>
struct hash<unhashable9>
{
	std::size_t operator()(const unhashable9 &) noexcept(true);
};

template <>
struct hash<unhashable10>
{
	std::size_t operator()(const unhashable10 &) const noexcept(true);
	~hash() noexcept(false) {}
};

template <>
struct hash<unhashable11>
{
	std::size_t operator()(const unhashable11 &) const noexcept(true);
	hash() noexcept(true);
	hash(const hash &);
	hash(hash &&) noexcept(false);
	hash &operator=(hash &&) noexcept(true);
	~hash() noexcept(true);
};

template <>
struct hash<unhashable12>
{
	std::size_t operator()(const unhashable12 &) const noexcept(true);
	hash() noexcept(true);
	hash(const hash &);
	hash(hash &&) noexcept(true);
	hash &operator=(hash &&) noexcept(false);
	~hash() noexcept(true);
};

template <>
struct hash<hashable1>
{
	std::size_t operator()(const hashable1 &) const noexcept(true);
};

template <>
struct hash<hashable2>
{
	std::size_t operator()(const hashable2 &) const noexcept(true);
};

template <>
struct hash<hashable3>
{
	hash() noexcept(true);
	std::size_t operator()(const hashable3 &) const noexcept(true);
};

template <>
struct hash<hashable4>
{
	std::size_t operator()(const hashable4 &) const noexcept(true);
	~hash() noexcept(true) {}
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
	BOOST_CHECK(!is_hashable<unhashable10>::value);
	BOOST_CHECK(!is_hashable<unhashable11>::value);
	BOOST_CHECK(!is_hashable<unhashable12>::value);
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
	hfo2() noexcept(true);
	std::size_t operator()(int) noexcept(true);
};

struct hfo3
{
	hfo3() noexcept(true);
	std::size_t operator()(int) const noexcept(true);
};

struct hfo4
{
	hfo4() noexcept(true);
	std::size_t operator()(int) const noexcept(true);
	~hfo4() noexcept(false);
};

struct hfo5
{
	hfo5() noexcept(true);
	std::size_t operator()(int) const;
};

struct hfo6
{
	hfo6() noexcept(true);
	hfo6(const hfo6 &) = delete;
	std::size_t operator()(int) const noexcept(true);
};

struct hfo7
{
	hfo7() noexcept(true);
	std::size_t operator()(int) const noexcept(true);
	hfo7(const hfo7 &);
	hfo7(hfo7 &&) noexcept(true);
	hfo7 &operator=(hfo7 &&) noexcept(true);
};

struct hfo8
{
	hfo8() noexcept(true);
	std::size_t operator()(int) const noexcept(true);
	hfo8(const hfo7 &);
	hfo8(hfo8 &&) noexcept(false);
	hfo8 &operator=(hfo8 &&) noexcept(true);
};

struct hfo9
{
	hfo9() noexcept(true);
	std::size_t operator()(int) const noexcept(true);
	hfo9(const hfo9 &);
	hfo9(hfo9 &&) noexcept(true);
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
	BOOST_CHECK((!is_hash_function_object<hfo9,int>::value));
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
	efo7(efo7 &&) noexcept(true);
	efo7 &operator=(const efo7 &);
	efo7 &operator=(efo7 &&) noexcept(true);
	bool operator()(int,int) const;
};

struct efo8
{
	efo8();
	efo8(const efo8 &);
	efo8(efo8 &&);
	efo8 &operator=(const efo8 &);
	efo8 &operator=(efo8 &&) noexcept(true);
	bool operator()(int,int) const;
};

struct efo9
{
	efo9();
	efo9(const efo9 &);
	efo9(efo9 &&) noexcept(true);
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
	BOOST_CHECK((!is_equality_function_object<efo8,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo9,int>::value));
	BOOST_CHECK((!is_equality_function_object<efo10,int>::value));
}

struct key01 {};

struct key02
{
	key02() = default;
	key02(const key02 &) = default;
	key02(key02 &&) noexcept(true);
	key02 &operator=(const key02 &) = default;
	key02 &operator=(key02 &&) noexcept(true);
	key02(const symbol_set &);
	bool operator==(const key02 &) const;
	bool operator!=(const key02 &) const;
	bool is_compatible(const symbol_set &) const noexcept(true);
	bool is_ignorable(const symbol_set &) const noexcept(true);
	key02 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key03
{
	key03() = default;
	key03(const key03 &) = default;
	key03(key03 &&) noexcept(true);
	key03 &operator=(const key03 &) = default;
	key03 &operator=(key03 &&) noexcept(true);
	key03(const symbol_set &);
	bool operator==(const key03 &) const;
	bool operator!=(const key03 &) const;
	bool is_compatible(const symbol_set &) const noexcept(true);
	bool is_ignorable(const symbol_set &) const noexcept(true);
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
	key04 &operator=(key04 &&) noexcept(true);
	key04(const symbol_set &);
	bool operator==(const key04 &) const;
	bool operator!=(const key04 &) const;
	bool is_compatible(const symbol_set &) const noexcept(true);
	bool is_ignorable(const symbol_set &) const noexcept(true);
	key04 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key05
{
	key05() = default;
	key05(const key05 &) = default;
	key05(key05 &&) noexcept(true);
	key05 &operator=(const key05 &) = default;
	key05 &operator=(key05 &&) noexcept(true);
	key05(const symbol_set &);
	bool operator==(const key05 &) const;
	bool operator!=(const key05 &) const;
	bool is_compatible(const symbol_set &) const;
	bool is_ignorable(const symbol_set &) const noexcept(true);
	key05 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key06
{
	key06() = default;
	key06(const key06 &) = default;
	key06(key06 &&) noexcept(true);
	key06 &operator=(const key06 &) = default;
	key06 &operator=(key06 &&) noexcept(true);
	key06(const symbol_set &);
	bool operator==(const key06 &) const;
	bool operator!=(const key06 &) const;
	bool is_compatible(const symbol_set &) const noexcept(true);
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
	key07(key07 &&) noexcept(true);
	key07 &operator=(const key07 &) = default;
	key07 &operator=(key07 &&) noexcept(true);
	key07(const symbol_set &);
	bool operator==(const key07 &) const;
	bool operator!=(const key07 &) const;
	bool is_compatible(const symbol_set &) const noexcept(true);
	bool is_ignorable(const symbol_set &) const noexcept(true);
	key07 merge_args(const symbol_set &, const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

struct key08
{
	key08() = default;
	key08(const key08 &) = default;
	key08(key08 &&) noexcept(true);
	key08 &operator=(const key08 &) = default;
	key08 &operator=(key08 &&) noexcept(true);
	key08(const symbol_set &);
	bool operator==(const key08 &) const;
	bool operator!=(const key08 &) const;
	bool is_compatible(const symbol_set &) const noexcept(true);
	bool is_ignorable(const symbol_set &) const noexcept(true);
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
	std::size_t operator()(const key02 &) const noexcept(true);
};

template <>
struct hash<key03> {};

template <>
struct hash<key04>
{
	std::size_t operator()(const key04 &) const noexcept(true);
};

template <>
struct hash<key05>
{
	std::size_t operator()(const key05 &) const noexcept(true);
};

template <>
struct hash<key06>
{
	std::size_t operator()(const key06 &) const noexcept(true);
};

template <>
struct hash<key07>
{
	std::size_t operator()(const key07 &) const noexcept(true);
};

template <>
struct hash<key08>
{
	std::size_t operator()(const key08 &) const noexcept(true);
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
	BOOST_CHECK(!is_key<key04>::value);
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
	cf02(cf02 &&) noexcept(true);
	cf02 &operator=(const cf02 &);
	cf02 &operator=(cf02 &&) noexcept(true);
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
	cf03(cf03 &&) noexcept(true);
	cf03 &operator=(const cf03 &);
	cf03 &operator=(cf03 &&) noexcept(true);
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
	cf04(cf04 &&) noexcept(true);
	cf04 &operator=(const cf04 &);
	cf04 &operator=(cf04 &&) noexcept(true);
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
	cf05(cf05 &&) noexcept(true);
	cf05 &operator=(const cf05 &);
	cf05 &operator=(cf05 &&) noexcept(true);
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
	cf06 &operator=(cf06 &&) noexcept(true);
	friend std::ostream &operator<<(std::ostream &, const cf06 &);
	cf06 operator-() const;
	bool operator==(const cf06 &) const;
	bool operator!=(const cf06 &) const;
	cf06 &operator+=(const cf06 &);
	cf06 &operator-=(const cf06 &);
	cf06 operator+(const cf06 &) const;
	cf06 operator-(const cf06 &) const;
};

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
	BOOST_CHECK(!is_cf<cf06>::value);
}
