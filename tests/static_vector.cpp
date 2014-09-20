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

#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"
#include "../src/type_traits.hpp"

// NOTE: here we define a custom string class base on std::string that respects nothrow requirements in hash_set:
// in the current GCC (4.6) the destructor of std::string does not have nothrow, so we cannot use it.
class custom_string: public std::string
{
	public:
		custom_string() = default;
		custom_string(const custom_string &) = default;
		// NOTE: strange thing here, move constructor of std::string results in undefined reference?
		custom_string(custom_string &&other) noexcept : std::string(other) {}
		template <typename... Args>
		custom_string(Args && ... params) : std::string(std::forward<Args>(params)...) {}
		custom_string &operator=(const custom_string &) = default;
		custom_string &operator=(custom_string &&other) noexcept
		{
			std::string::operator=(std::move(other));
			return *this;
		}
		~custom_string() {}
};

namespace std
{

template <>
struct hash<custom_string>
{
	size_t operator()(const custom_string &s) const noexcept
	{
		return hash<std::string>()(s);
	}
};

}

using namespace piranha;

static_assert(is_hashable<custom_string>::value,"cc");

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

BOOST_AUTO_TEST_CASE(static_vector_size_type_test)
{
	BOOST_CHECK((std::is_same<detail::static_vector_size_type<10u>::type,unsigned char>::value));
	BOOST_CHECK((std::is_same<detail::static_vector_size_type<255u>::type,unsigned char>::value));
	BOOST_CHECK((std::is_same<detail::static_vector_size_type<10000u>::type,unsigned char>::value) ||
		(std::is_same<detail::static_vector_size_type<10000u>::type,unsigned short>::value));
	BOOST_CHECK((std::is_same<detail::static_vector_size_type<4294967295ul>::type,unsigned char>::value) ||
		(std::is_same<detail::static_vector_size_type<4294967295ul>::type,unsigned short>::value) ||
		(std::is_same<detail::static_vector_size_type<4294967295ul>::type,unsigned>::value) ||
		(std::is_same<detail::static_vector_size_type<4294967295ul>::type,unsigned long>::value)
		);
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

// Class that throws after a few default constructions.
struct time_bomb
{
	time_bomb():m_vector(5)
	{
		if (s_counter == 2u) {
			throw std::runtime_error("ka-pow!");
		}
		++s_counter;
	}
	time_bomb(time_bomb &&) = default;
	time_bomb(const time_bomb &) = default;
	time_bomb &operator=(time_bomb &&other) noexcept
	{
		m_vector = std::move(other.m_vector);
		return *this;
	}
	bool operator==(const time_bomb &other) const
	{
		return m_vector == other.m_vector;
	}
	~time_bomb() {}
	std::vector<int> m_vector;
	static unsigned s_counter;
};

unsigned time_bomb::s_counter = 0u;

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
			if (U::value < 3u) {
				return;
			}
			typedef static_vector<time_bomb,U::value> vector_type2;
			time_bomb::s_counter = 0u;
			vector_type2 v2;
			v2.resize(1);
			v2.resize(2);
			BOOST_CHECK_THROW(v2.resize(3),std::runtime_error);
			BOOST_CHECK_EQUAL(v2.size(),2u);
			time_bomb::s_counter = 0u;
			BOOST_CHECK(v2[0] == time_bomb());
			BOOST_CHECK(v2[1] == time_bomb());
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

struct hash_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v1;
			BOOST_CHECK(v1.hash() == 0u);
			v1.push_back(T());
			BOOST_CHECK(v1.hash() == std::hash<T>()(T()));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_hash_test)
{
	boost::mpl::for_each<value_types>(hash_tester());
}

// Move semantics tester.
struct move_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef static_vector<T,U::value> vector_type;
			vector_type v1;
			v1.push_back(T());
			vector_type v2(std::move(v1));
			BOOST_CHECK_EQUAL(T(),v2[0u]);
			BOOST_CHECK_EQUAL(v1.size(),0u);
			BOOST_CHECK(v1.begin() == v1.end());
			v1 = std::move(v2);
			BOOST_CHECK_EQUAL(T(),v1[0u]);
			BOOST_CHECK_EQUAL(v2.size(),0u);
			BOOST_CHECK(v2.begin() == v2.end());
			if (U::value > 1u) {
				v1.push_back(boost::lexical_cast<T>("2"));
				v1.push_back(boost::lexical_cast<T>("3"));
				vector_type v3(std::move(v1));
				BOOST_CHECK_EQUAL(v3.size(),3u);
				BOOST_CHECK_EQUAL(v3[0u],T());
				BOOST_CHECK_EQUAL(v3[1u],boost::lexical_cast<T>("2"));
				BOOST_CHECK_EQUAL(v3[2u],boost::lexical_cast<T>("3"));
				BOOST_CHECK_EQUAL(v1.size(),0u);
				BOOST_CHECK(v1.begin() == v1.end());
				v1 = std::move(v3);
				BOOST_CHECK_EQUAL(v1.size(),3u);
				BOOST_CHECK_EQUAL(v1[0u],T());
				BOOST_CHECK_EQUAL(v1[1u],boost::lexical_cast<T>("2"));
				BOOST_CHECK_EQUAL(v1[2u],boost::lexical_cast<T>("3"));
				BOOST_CHECK_EQUAL(v3.size(),0u);
				BOOST_CHECK(v3.begin() == v3.end());
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(static_vector_move_semantics_test)
{
	boost::mpl::for_each<value_types>(move_tester());
}
