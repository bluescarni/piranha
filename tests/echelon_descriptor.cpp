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

#include "../src/echelon_descriptor.hpp"

#define BOOST_TEST_MODULE echelon_descriptor_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "../src/base_term.hpp"
#include "../src/config.hpp"
#include "../src/integer.hpp"
#include "../src/monomial.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/symbol.hpp"
#include "../src/utils.hpp"

// TODO: test with larger echelon sizes. Make sure in higher sizes there is term_type1, so
// that tests below remain valid.

using namespace piranha;

template <int N>
class term_type_n: public base_term<numerical_coefficient<double>,monomial<int>,term_type_n<N>>
{
	public:
		term_type_n() = default;
		term_type_n(const term_type_n &) = default;
		term_type_n(term_type_n &&) = default;
		term_type_n &operator=(const term_type_n &) = default;
		term_type_n &operator=(term_type_n &&other) piranha_noexcept_spec(true)
		{
			base_term<numerical_coefficient<double>,monomial<int>,term_type_n>::operator=(std::move(other));
			return *this;
		}
		// Needed to satisfy concept checking.
		explicit term_type_n(const numerical_coefficient<double> &, const monomial<int> &) {}
};

typedef term_type_n<0> term_type0;
typedef term_type_n<1> term_type1;

typedef boost::mpl::vector<term_type1> term_types;

struct constructor_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		// Default constructor.
		typedef echelon_descriptor<T> ed_type;
		// Copy constructor.
		ed_type a, b = a;
		// Move constructor.
		ed_type ed1;
		ed_type ed2(std::move(ed1));
		// Copy assignment.
		ed1 = b;
		// Move assignment.
		ed2 = std::move(a);
		// Copy constructor from different term type.
		typedef echelon_descriptor<term_type0> ed_type0;
		ed_type0 a0;
		ed_type c{a0};
		// Move constructor from different term type.
		ed_type d{std::move(a0)};
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_constructor_test)
{
	boost::mpl::for_each<term_types>(constructor_tester());
}

struct getters_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		typedef echelon_descriptor<T> ed_type;
		ed_type a;
		a.get_args_tuple();
		a.template get_args<term_type1>();
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_getters_test)
{
	boost::mpl::for_each<term_types>(getters_tester());
}

struct add_symbol_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		typedef echelon_descriptor<T> ed_type;
		ed_type a;
		BOOST_CHECK(a.template get_args<term_type1>() == std::vector<symbol>{});
		a.template add_symbol<term_type1>(symbol("c"));
		a.template add_symbol<term_type1>(symbol("b"));
		a.template add_symbol<term_type1>("a");
		a.template add_symbol<term_type1>("d");
		BOOST_CHECK(a.template get_args<term_type1>() == std::vector<symbol>({symbol("a"),symbol("b"),symbol("c"),symbol("d")}));
		// Check that adding existing symbol results in an error.
		BOOST_CHECK_THROW(a.template add_symbol<term_type1>(symbol("d")),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_add_symbol_test)
{
	boost::mpl::for_each<term_types>(add_symbol_tester());
}

struct diff_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		typedef echelon_descriptor<T> ed_type;
		typedef typename ed_type::top_level_term_type tl_type;
		ed_type a, b;
		// Test with two empty objects.
		auto diff = a.difference(ed_type{});
		auto default_checker = [](const typename std::tuple_element<0,decltype(diff)>::type &arg) -> void {
			BOOST_CHECK_EQUAL(arg.size(),std::size_t(1));
			// Avoid undefined behaviour if the test above fails.
			if (arg.size()) {
				BOOST_CHECK_EQUAL(arg[0].size(),std::size_t(0));
			}
		};
		tuple_iterate<decltype(default_checker),decltype(diff)>()(default_checker,diff);
		// Test with the example shown in the docs.
		a.template add_symbol<tl_type>(symbol("e"));
		a.template add_symbol<tl_type>(symbol("c"));
		b.template add_symbol<tl_type>(symbol("a"));
		b.template add_symbol<tl_type>(symbol("b"));
		b.template add_symbol<tl_type>(symbol("f"));
		b.template add_symbol<tl_type>(symbol("c"));
		diff = a.difference(b);
		typename std::tuple_element<0,decltype(diff)>::type expected_result = {{0,1},{},{3}};
		BOOST_CHECK(std::get<0>(diff) == expected_result);
		// Another example.
		BOOST_CHECK(std::get<0>(ed_type{}.difference(b)) == decltype(expected_result)({{0,1,2,3}}));
		// Another example.
		BOOST_CHECK_EQUAL(std::get<0>(b.difference(ed_type{})).size(),unsigned(5));
		BOOST_CHECK(std::get<0>(b.difference(ed_type{}))[0] == std::vector<std::vector<symbol>::size_type>());
		BOOST_CHECK(std::get<0>(b.difference(ed_type{}))[1] == std::vector<std::vector<symbol>::size_type>());
		BOOST_CHECK(std::get<0>(b.difference(ed_type{}))[2] == std::vector<std::vector<symbol>::size_type>());
		BOOST_CHECK(std::get<0>(b.difference(ed_type{}))[3] == std::vector<std::vector<symbol>::size_type>());
		BOOST_CHECK(std::get<0>(b.difference(ed_type{}))[4] == std::vector<std::vector<symbol>::size_type>());
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_diff_test)
{
	boost::mpl::for_each<term_types>(diff_tester());
}

struct merge_tester
{
	template <typename T>
	void operator()(const T &) const
	{
		typedef echelon_descriptor<T> ed_type;
		typedef typename ed_type::top_level_term_type tl_type;
		ed_type a, b;
		auto new_ed = a.merge(b).first;
		BOOST_CHECK(a.get_args_tuple() == new_ed.get_args_tuple());
		BOOST_CHECK(b.get_args_tuple() == new_ed.get_args_tuple());
		a.template add_symbol<tl_type>(symbol("e"));
		a.template add_symbol<tl_type>(symbol("c"));
		new_ed = a.merge(b).first;
		BOOST_CHECK(new_ed.get_args_tuple() == a.get_args_tuple());
		new_ed = b.merge(a).first;
		BOOST_CHECK(new_ed.get_args_tuple() == a.get_args_tuple());
		b.template add_symbol<tl_type>(symbol("a"));
		b.template add_symbol<tl_type>(symbol("b"));
		b.template add_symbol<tl_type>(symbol("f"));
		b.template add_symbol<tl_type>(symbol("c"));
		auto new_a = a.merge(b).first;
		auto new_b = b.merge(new_a).first;
		BOOST_CHECK(new_a.get_args_tuple() == new_b.get_args_tuple());
		auto saved_new = new_a;
		new_b = b.merge(a).first;
		new_a = a.merge(new_b).first;
		BOOST_CHECK(new_a.get_args_tuple() == new_b.get_args_tuple());
		BOOST_CHECK(new_a.get_args_tuple() == saved_new.get_args_tuple());
	}
};

BOOST_AUTO_TEST_CASE(echelon_descriptor_merge_test)
{
	boost::mpl::for_each<term_types>(merge_tester());
}
