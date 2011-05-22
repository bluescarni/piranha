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

#include "../src/numerical_coefficient.hpp"

#define BOOST_TEST_MODULE numerical_coefficient_test
#include <boost/test/unit_test.hpp>

#include <boost/concept/assert.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <type_traits>

#include "../src/base_term.hpp"
#include "../src/concepts/coefficient.hpp"
#include "../src/echelon_descriptor.hpp"
#include "../src/integer.hpp"
#include "../src/math.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/mf_int.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,mf_int,integer> types;

typedef float other_type;

struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef numerical_coefficient<T> nc;
		typedef echelon_descriptor<polynomial_term<nc,int>> ed_type;
		ed_type ed;
		BOOST_CONCEPT_ASSERT((concept::Coefficient<nc>));
		BOOST_CHECK((std::has_trivial_destructor<T>::value && std::has_trivial_destructor<nc>::value) || (!std::has_trivial_destructor<T>::value && !std::has_trivial_destructor<nc>::value));
		BOOST_CHECK((std::has_trivial_copy_constructor<T>::value && std::has_trivial_copy_constructor<nc>::value) || (!std::has_trivial_copy_constructor<T>::value && !std::has_trivial_copy_constructor<nc>::value));
		// Default constructor.
		BOOST_CHECK_EQUAL(nc().get_value(),T());
		// Copy construction from value.
		T value(3);
		BOOST_CHECK_EQUAL(nc(value,ed).get_value(),T(3));
		// Copy construction from different value type.
		other_type other_value(3);
		BOOST_CHECK_EQUAL(nc(other_value,ed).get_value(),T(3));
		// Move construction from value.
		BOOST_CHECK_EQUAL(nc(T(3),ed).get_value(),T(3));
		// Move construciton from other value type.
		BOOST_CHECK_EQUAL(nc(other_type(3),ed).get_value(),T(3));
		// Copy construction.
		nc other(value,ed);
		BOOST_CHECK_EQUAL(nc(other,ed).get_value(),T(3));
		// Move construction.
		BOOST_CHECK_EQUAL(nc(nc(value,ed)).get_value(),T(3));
		// Move assignment.
		other = nc(T(3),ed);
		// Check copy/move construction from coefficients of different types.
		typedef numerical_coefficient<int> nc_other;
		typedef echelon_descriptor<polynomial_term<nc_other,int>> ed_type_other;
		ed_type_other ed_other;
		nc_other nco{3,ed_other};
		BOOST_CHECK_EQUAL(nc(nco,ed).get_value(),T(3));
		BOOST_CHECK_EQUAL(nc(nc_other(3,ed_other),ed).get_value(),T(3));
		// Assignment from int.
		nc n;
		n = 10;
		BOOST_CHECK_EQUAL(n.get_value(),T(10));
		n = int(10);
		BOOST_CHECK_EQUAL(n.get_value(),T(10));
		// Assignment from numerical_coefficient of same type.
		n = other;
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
		n = nc(value,ed);
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
		// Assignment from numerical_coefficient of different type.
		n = nco;
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
		n = nc_other(3,ed_other);
		BOOST_CHECK_EQUAL(n.get_value(),T(3));
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_constructor_test)
{
	boost::mpl::for_each<types>(constructor_tester());
}

struct ignorability_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef numerical_coefficient<T> nc;
		typedef echelon_descriptor<polynomial_term<numerical_coefficient<T>,int>> ed_type;
		ed_type ed;
		BOOST_CHECK((nc(0,ed).is_ignorable(ed) && math::is_zero(T(0))) || (!nc(0,ed).is_ignorable(ed) && !math::is_zero(T(0))));
		BOOST_CHECK((!nc(1,ed).is_ignorable(ed) && !math::is_zero(T(1))) || (nc(1,ed).is_ignorable(ed) && math::is_zero(T(1))));
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_ignorability_test)
{
	boost::mpl::for_each<types>(ignorability_tester());
}

struct compatibility_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef numerical_coefficient<T> nc;
		typedef echelon_descriptor<polynomial_term<numerical_coefficient<T>,int>> ed_type;
		ed_type ed;
		BOOST_CHECK(nc().is_compatible(ed));
		BOOST_CHECK(nc(1,ed).is_compatible(ed));
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_compatibility_test)
{
	boost::mpl::for_each<types>(compatibility_tester());
}

struct arithmetics_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef numerical_coefficient<T> nc;
		typedef numerical_coefficient<other_type> nc_other;
		typedef echelon_descriptor<polynomial_term<numerical_coefficient<T>,int>> ed_type;
		typedef echelon_descriptor<polynomial_term<numerical_coefficient<other_type>,int>> ed_type_other;
		ed_type ed;
		ed_type_other ed_other;
		nc cont;
		T value{};
		// Same numerical container.
		cont.add(nc{T(1),ed},ed);
		value += T(1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		cont.subtract(nc{T(-1),ed},ed);
		value -= T(-1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		// Other numerical container.
		cont.add(nc_other{other_type(1),ed_other},ed);
		value += other_type(1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		cont.subtract(nc_other{other_type(1),ed_other},ed);
		value -= other_type(1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		// Same numerical type.
		cont.add(T(1),ed);
		value += T(1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		cont.subtract(T(-1),ed);
		value -= T(-1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		// Other numerical type.
		cont.add(other_type(1),ed);
		value += other_type(1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		cont.subtract(other_type(-1),ed);
		value -= other_type(-1);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		// Negate.
		cont.negate(ed);
		math::negate(value);
		BOOST_CHECK_EQUAL(cont.get_value(),value);
		// In-place multiplication.
		nc cont2{T(1),ed};
		cont2.multiply_by(2,ed);
		BOOST_CHECK_EQUAL(cont2.get_value(),T(1) * 2);
		cont2.multiply_by(nc{T(2),ed},ed);
		BOOST_CHECK_EQUAL(cont2.get_value(),(T(1) * 2) * T(2));
		nc cont3{T(3),ed};
		cont2.multiply_by(cont3,ed);
		BOOST_CHECK_EQUAL(cont2.get_value(),((T(1) * 2) * T(2)) * T(3));
		cont2.multiply_by(nc_other{-1,ed},ed);
		BOOST_CHECK_EQUAL(cont2.get_value(),(((T(1) * 2) * T(2)) * T(3)) * other_type(-1));
		// Multiplication.
		BOOST_CHECK_EQUAL((nc{T(1),ed}.multiply(2,ed).get_value()),T(1) * 2);
		BOOST_CHECK_EQUAL((nc{T(1),ed}.multiply(nc{T(2),ed},ed).get_value()),T(1) * T(2));
		nc cont4{T(4),ed};
		BOOST_CHECK_EQUAL((nc{T(2),ed}.multiply(cont4,ed).get_value()),T(2) * T(4));
		BOOST_CHECK_EQUAL((nc{T(2),ed}.multiply(nc_other{-1,ed},ed).get_value()),T(2) * other_type(-1));
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_arithmetics_test)
{
	boost::mpl::for_each<types>(arithmetics_tester());
}

struct binary_op_promotion_rule_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef numerical_coefficient<T> nc1_type;
			typedef numerical_coefficient<U> nc2_type;
			BOOST_CHECK((binary_op_promotion_rule<nc1_type,nc2_type>::value == binary_op_promotion_rule<T,U>::value));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_binary_op_promotion_rule_test)
{
	boost::mpl::for_each<types>(binary_op_promotion_rule_tester());
}

struct equality_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef numerical_coefficient<T> nc;
		typedef echelon_descriptor<polynomial_term<numerical_coefficient<T>,int>> ed_type;
		ed_type ed;
		BOOST_CHECK(nc{}.is_equal_to(nc(0,ed),ed));
		BOOST_CHECK(nc(3,ed).is_equal_to(T(3),ed));
		BOOST_CHECK(nc(3,ed).is_equal_to(numerical_coefficient<float>(3,ed),ed));
	}
};

BOOST_AUTO_TEST_CASE(numerical_coefficient_equality_test)
{
	boost::mpl::for_each<types>(equality_tester());
}
