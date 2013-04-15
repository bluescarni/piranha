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

#include "../src/static_vector.hpp"

#define BOOST_TEST_MODULE static_vector_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>
#include <sstream>
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/type_traits.hpp"

// NOTE: here we define a custom string class base on std::string that respects nothrow requirements in hash_set:
// in the current GCC (4.6) the destructor of std::string does not have nothrow, so we cannot use it.
class custom_string: public std::string
{
	public:
		custom_string() = default;
		custom_string(const custom_string &) = default;
		// NOTE: strange thing here, move constructor of std::string results in undefined reference?
		custom_string(custom_string &&other) noexcept(true) : std::string(other) {}
		template <typename... Args>
		custom_string(Args && ... params) : std::string(std::forward<Args>(params)...) {}
		custom_string &operator=(const custom_string &) = default;
		custom_string &operator=(custom_string &&other) noexcept(true)
		{
			std::string::operator=(std::move(other));
			return *this;
		}
		~custom_string() noexcept(true) {}
};

using namespace piranha;

typedef boost::mpl::vector<int,integer,custom_string> value_types;
typedef boost::mpl::vector<std::integral_constant<std::uint_least8_t,1u>,std::integral_constant<std::uint_least8_t,5u>,
	std::integral_constant<std::uint_least8_t,10u>> size_types;

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			// Default constructor.
			vector_type v;
			BOOST_CHECK_EQUAL(v.size(),0u);
			BOOST_CHECK_EQUAL(vector_type(v).size(),0u);
			BOOST_CHECK_EQUAL(vector_type(std::move(v)).size(),0u);
			v = vector_type();
			v.push_back(boost::lexical_cast<T>(1));
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(1));
			// Copy constructor.
			BOOST_CHECK_EQUAL(vector_type(v).size(),1u);
			BOOST_CHECK_EQUAL(vector_type(v)[0],boost::lexical_cast<T>(1));
			// Move constructor.
			BOOST_CHECK_EQUAL(vector_type(std::move(v))[0],boost::lexical_cast<T>(1));
			// Copy assignment.
			vector_type tmp;
			tmp.push_back(boost::lexical_cast<T>(1));
			v = tmp;
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(1));
			// Move assignment.
			v = vector_type();
			v.push_back(boost::lexical_cast<T>(1));
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(1));
			// Mutating accessor.
			v[0] = boost::lexical_cast<T>(2);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(2));
			if (U::value > 1u) {
				// Move Assignment with different sizes.
				vector_type v, u;
				v.push_back(boost::lexical_cast<T>(1));
				v.push_back(boost::lexical_cast<T>(2));
				u.push_back(boost::lexical_cast<T>(3));
				v = std::move(u);
				BOOST_CHECK_EQUAL(v.size(),1u);
				BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(3));
				u = vector_type();
				v = vector_type();
				v.push_back(boost::lexical_cast<T>(1));
				v.push_back(boost::lexical_cast<T>(2));
				u.push_back(boost::lexical_cast<T>(3));
				u = std::move(v);
				BOOST_CHECK_EQUAL(u.size(),2u);
				BOOST_CHECK_EQUAL(u[0],boost::lexical_cast<T>(1));
				BOOST_CHECK_EQUAL(u[1],boost::lexical_cast<T>(2));
			}
			// Constructor from copies.
			v = vector_type(0u,boost::lexical_cast<T>(1));
			BOOST_CHECK_EQUAL(v.size(),0u);
			v = vector_type(1u,boost::lexical_cast<T>(2));
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK(v[0u] == boost::lexical_cast<T>(2));
			BOOST_CHECK_THROW(v = vector_type(1u + U::value,boost::lexical_cast<T>(2)),std::bad_alloc);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_constructor_test)
{
	environment env;
	boost::mpl::for_each<value_types>(constructor_tester());
}

struct iterator_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v;
			BOOST_CHECK(v.begin() == v.end());
			v.push_back(boost::lexical_cast<T>(1));
			auto it = v.begin();
			++it;
			BOOST_CHECK(it == v.end());
			BOOST_CHECK_EQUAL(std::distance(v.begin(),v.end()),1);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_iterator_test)
{
	boost::mpl::for_each<value_types>(iterator_tester());
}

struct equality_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			BOOST_CHECK(vector_type() == vector_type());
			vector_type v1, v2;
			v1.push_back(boost::lexical_cast<T>(1));
			BOOST_CHECK(!(v1 == v2));
			BOOST_CHECK(v1 != v2);
			v2.push_back(boost::lexical_cast<T>(1));
			BOOST_CHECK(v1 == v2);
			BOOST_CHECK(!(v1 != v2));
			v1 = vector_type();
			v1.push_back(boost::lexical_cast<T>(2));
			BOOST_CHECK(!(v1 == v2));
			BOOST_CHECK(v1 != v2);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_equality_test)
{
	boost::mpl::for_each<value_types>(equality_tester());
}

struct push_back_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v;
			// Move-pushback.
			v.push_back(boost::lexical_cast<T>(1));
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(1));
			// Copy-pushback.
			auto tmp = boost::lexical_cast<T>(1);
			v = vector_type();
			v.push_back(tmp);
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(1));
			// Check for throw.
			for (auto i = v.size(); i < U::value; ++i) {
				v.push_back(tmp);
			}
			BOOST_CHECK_THROW(v.push_back(tmp),std::bad_alloc);
			BOOST_CHECK_THROW(v.push_back(std::move(tmp)),std::bad_alloc);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_push_back_test)
{
	boost::mpl::for_each<value_types>(push_back_tester());
}

struct emplace_back_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v;
			v.emplace_back(boost::lexical_cast<T>(1));
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],boost::lexical_cast<T>(1));
			// Check for throw.
			for (auto i = v.size(); i < U::value; ++i) {
				v.emplace_back(boost::lexical_cast<T>(1));
			}
			BOOST_CHECK_THROW(v.emplace_back(boost::lexical_cast<T>(1)),std::bad_alloc);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_emplace_back_test)
{
	boost::mpl::for_each<value_types>(emplace_back_tester());
}

struct resize_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v;
			v.resize(1u);
			BOOST_CHECK_THROW(v.resize(U::value + 1u),std::bad_alloc);
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],T());
			v.resize(1u);
			BOOST_CHECK_EQUAL(v.size(),1u);
			BOOST_CHECK_EQUAL(v[0],T());
			v.resize(0u);
			BOOST_CHECK_EQUAL(v.size(),0u);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_resize_test)
{
	boost::mpl::for_each<value_types>(resize_tester());
}

struct stream_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v;
			std::ostringstream oss1;
			oss1 << v;
			BOOST_CHECK(!oss1.str().empty());
			v.push_back(boost::lexical_cast<T>(1));
			if (U::value > 1u) {
				v.push_back(boost::lexical_cast<T>(1));
			}
			std::ostringstream oss2;
			oss2 << v;
			BOOST_CHECK(!oss2.str().empty());
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_stream_test)
{
	boost::mpl::for_each<value_types>(stream_tester());
}

struct type_traits_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			BOOST_CHECK(is_container_element<vector_type>::value);
			BOOST_CHECK(is_ostreamable<vector_type>::value);
			BOOST_CHECK(is_equality_comparable<vector_type>::value);
			BOOST_CHECK(!is_addable<vector_type>::value);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_type_traits_test)
{
	boost::mpl::for_each<value_types>(type_traits_tester());
}
